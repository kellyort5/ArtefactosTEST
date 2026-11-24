// Sesión Telnet simulada. Lógica pura: no usa sockets ni FreeRTOS y se prueba
// en host. La línea que escribe el cliente nunca se almacena: solo se cuenta
// su longitud, así que no hay copia de usuario ni contraseña que pueda salir
// a logs, eventos o memoria persistente. Nada se interpreta ni se ejecuta.
#include "include/cebo.hpp"
#include <cstring>

namespace sentinel::cebo {
namespace {
constexpr uint8_t Iac = 255, Dont = 254, Do = 253, Wont = 252, Will = 251,
                  Sb = 250, Se = 240, Echo = 1;
constexpr unsigned MaxReplies = 16;  // negociaciones respondidas por sesión
constexpr size_t MaxSubnegotiation = 64;

// Textos fijos. Los literales se separan tras cada escape hexadecimal para que
// el siguiente carácter no se interprete como parte de él.
#define FIXED(name, literal)                                                   \
  const uint8_t name[] = literal;                                              \
  constexpr size_t name##Length = sizeof(name) - 1
FIXED(Banner, "\r\nRouter management console\r\n\r\n"
              "login: ");
FIXED(PasswordPrompt, "\xff\xfb\x01"
                      "Password: "); // IAC WILL ECHO: el cliente no muestra la clave
FIXED(LoginFailed, "\xff\xfc\x01"
                   "\r\nLogin incorrect\r\n\r\n"
                   "login: "); // IAC WONT ECHO
FIXED(TooManyAttempts, "\xff\xfc\x01"
                       "\r\nLogin incorrect\r\n\r\n"
                       "Too many failed attempts\r\n");
FIXED(InputTooLong, "\r\nInput too long\r\n");
FIXED(SessionLimit, "\r\nSession limit reached\r\n");
#undef FIXED
} // namespace

void TelnetSession::open(const Peer &peer, uint64_t now_ms,
                         InteractionBus &bus) {
  *this = TelnetSession();
  peer_ = peer;
  bus_ = &bus;
  now_ = last_ms_ = now_ms;
  record(Kind::Connect);
  emit(Banner, BannerLength);
}

void TelnetSession::record(Kind kind, Close close) {
  if (!bus_)
    return;
  Interaction i;
  memcpy(i.ipv4, peer_.ipv4, sizeof(i.ipv4));
  i.source_port = peer_.port;
  i.destination_port = TelnetPort;
  i.protocol = Protocol::Telnet;
  i.kind = kind;
  i.path = Path::None; // el contrato exige ruta nula en Telnet
  i.close = close;
  i.uptime_ms = now_;
  bus_->push(i);
}

void TelnetSession::emit(const uint8_t *bytes, size_t length) {
  if (out_length_ + length > sizeof(out_)) {
    // Un cliente que encadena respuestas más rápido de lo que se envían se
    // corta en lugar de acumular memoria.
    closing_ = true;
    if (reason_ == Close::None)
      reason_ = Close::Limit;
    return;
  }
  memcpy(out_ + out_length_, bytes, length);
  out_length_ += length;
}

void TelnetSession::close_with(Close reason) {
  closing_ = true;
  reason_ = reason;
}

void TelnetSession::negotiate(uint8_t command, uint8_t option) {
  // DO/DONT ECHO responden a nuestra oferta durante la contraseña: sin acción.
  if (option == Echo && (command == Do || command == Dont))
    return;
  if (replies_ >= MaxReplies)
    return;
  // Se rechaza cualquier opción: WILL x -> DONT x, DO x -> WONT x. WONT y
  // DONT no se responden, para no abrir bucles de negociación.
  uint8_t reply[3] = {Iac, 0, option};
  if (command == Will)
    reply[1] = Dont;
  else if (command == Do)
    reply[1] = Wont;
  else
    return;
  ++replies_;
  emit(reply, sizeof(reply));
}

void TelnetSession::end_line() {
  line_ = 0;
  if (stage_ == Stage::User) {
    record(Kind::Input);
    stage_ = Stage::Password;
    emit(PasswordPrompt, PasswordPromptLength);
    return;
  }
  record(Kind::AuthAttempt);
  stage_ = Stage::User;
  if (++attempts_ >= TelnetMaxAttempts) {
    emit(TooManyAttempts, TooManyAttemptsLength);
    close_with(Close::Limit);
  } else {
    emit(LoginFailed, LoginFailedLength);
  }
}

void TelnetSession::data_byte(uint8_t byte) {
  // Fin de línea Telnet: CR LF, CR NUL, LF o CR solos; el segundo byte de la
  // pareja no cuenta como línea nueva aunque llegue en otro fragmento.
  if (after_cr_) {
    after_cr_ = false;
    if (byte == '\n' || byte == 0)
      return;
  }
  if (byte == '\r') {
    after_cr_ = true;
    end_line();
    return;
  }
  if (byte == '\n') {
    end_line();
    return;
  }
  if (byte == 8 || byte == 127) { // borrado: la longitud nunca baja de cero
    if (line_)
      --line_;
    return;
  }
  if (++line_ > TelnetMaxLine) {
    // La línea excesiva cuenta como la entrada de su etapa y se rechaza.
    record(stage_ == Stage::User ? Kind::Input : Kind::AuthAttempt);
    emit(InputTooLong, InputTooLongLength);
    close_with(Close::Limit);
  }
}

void TelnetSession::step(uint8_t byte) {
  switch (parser_) {
  case Parser::Data:
    if (byte == Iac)
      parser_ = Parser::Iac;
    else
      data_byte(byte);
    break;
  case Parser::Iac:
    if (byte == Iac) { // IAC IAC es el dato 255
      parser_ = Parser::Data;
      data_byte(byte);
    } else if (byte >= Will && byte <= Dont) {
      command_ = byte;
      parser_ = Parser::Option;
    } else if (byte == Sb) {
      sub_ = 0;
      parser_ = Parser::Sub;
    } else {
      parser_ = Parser::Data; // NOP, AYT, etc.: se ignoran
    }
    break;
  case Parser::Option:
    negotiate(command_, byte);
    parser_ = Parser::Data;
    break;
  case Parser::Sub:
    if (byte == Iac)
      parser_ = Parser::SubIac;
    else if (++sub_ > MaxSubnegotiation)
      parser_ = Parser::Data; // no se traga datos de forma indefinida
    break;
  case Parser::SubIac:
    parser_ = byte == Se ? Parser::Data : Parser::Sub;
    break;
  }
}

bool TelnetSession::feed(const uint8_t *data, size_t length, uint64_t now_ms) {
  if (closing_ || finished_)
    return false;
  now_ = last_ms_ = now_ms;
  for (size_t n = 0; n < length && !closing_; ++n) {
    if (++bytes_ > TelnetMaxSessionBytes) {
      emit(SessionLimit, SessionLimitLength);
      close_with(Close::Limit);
      break;
    }
    step(data[n]);
  }
  return !closing_;
}

bool TelnetSession::idle(uint64_t now_ms, uint32_t timeout_ms) const {
  return !finished_ && now_ms - last_ms_ >= timeout_ms;
}

void TelnetSession::finish(Close reason, uint64_t now_ms) {
  if (finished_)
    return;
  finished_ = true;
  closing_ = true;
  now_ = now_ms;
  record(Kind::Disconnect, reason == Close::None ? Close::Peer : reason);
}

} // namespace sentinel::cebo
