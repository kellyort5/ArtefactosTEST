# Contexto de construcción: la raíz del repositorio (ver compose.yaml). El
# componente consume el paquete compartido de contratos (`file:../contracts`),
# que se instala y compila desde el lockfile del repositorio.
# syntax=docker/dockerfile:1
FROM node:24-bookworm-slim AS build
WORKDIR /repo
# Solo los manifiestos necesarios para que `npm ci --workspace contracts` valide el lockfile.
COPY package.json package-lock.json ./
COPY app/package.json app/package.json
COPY contracts contracts
RUN npm ci --workspace contracts --no-audit --no-fund \
 && npm run build --workspace contracts
COPY notifications/package.json notifications/package-lock.json notifications/
WORKDIR /repo/notifications
RUN npm ci --omit=dev --no-audit --no-fund
COPY notifications/src src
COPY notifications/scripts scripts

FROM node:24-bookworm-slim
ENV NODE_ENV=production
WORKDIR /repo
# ajv (dependencia de contratos) está en la raíz del repositorio.
COPY --from=build /repo/node_modules node_modules
COPY --from=build /repo/contracts contracts
COPY --from=build /repo/notifications notifications
# Directorio de estado privado, propiedad del usuario sin privilegios.
RUN mkdir -p /var/lib/sentinel-notifications && chown node:node /var/lib/sentinel-notifications
USER node
WORKDIR /repo/notifications
EXPOSE 8443
CMD ["node", "src/index.js"]
