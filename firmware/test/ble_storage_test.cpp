#include <nvs.h>
#include <host/ble_store.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "ble_storage.h"
struct Record { std::string key; std::vector<unsigned char> data; unsigned type=NVS_TYPE_BLOB; };
std::vector<Record> records;
size_t cursor=0;int opened=0,closed=0;esp_err_t open_error=ESP_OK;
esp_err_t nvs_open(const char *ns,int mode,nvs_handle_t *h){assert(!strcmp(ns,"nimble_bond")&&mode==NVS_READONLY);if(open_error)return open_error;++opened;*h=1;return ESP_OK;}
void nvs_close(nvs_handle_t){++closed;}
esp_err_t nvs_entry_find(const char *p,const char *ns,nvs_type_t t,nvs_iterator_t *i){assert(!strcmp(p,"nvs")&&!strcmp(ns,"nimble_bond")&&t==NVS_TYPE_ANY);cursor=0;*i=records.empty()?nullptr:&cursor;return records.empty()?ESP_ERR_NVS_NOT_FOUND:ESP_OK;}
esp_err_t nvs_entry_info(nvs_iterator_t,nvs_entry_info_t *i){strcpy(i->key,records[cursor].key.c_str());i->type=records[cursor].type;return ESP_OK;}
esp_err_t nvs_entry_next(nvs_iterator_t *i){if(++cursor==records.size()){*i=nullptr;return ESP_ERR_NVS_NOT_FOUND;}return ESP_OK;}
void nvs_release_iterator(nvs_iterator_t){}
esp_err_t nvs_get_blob(nvs_handle_t,const char *key,void *out,size_t *n){for(auto&r:records)if(r.key==key){if(out){assert(*n>=r.data.size());memcpy(out,r.data.data(),r.data.size());}*n=r.data.size();return ESP_OK;}return ESP_ERR_NVS_NOT_FOUND;}
void add(const std::string &key,bool authenticated=true,bool sc=true,int bits=16){ble_store_value_sec v={};v.key_size=bits;v.authenticated=authenticated;v.sc=sc;Record r{key,std::vector<unsigned char>(sizeof(v))};memcpy(r.data.data(),&v,sizeof(v));records.push_back(r);}
void check(int expected,const char* name){auto before=records;int o=opened,c=closed;assert(sentinel::ble_storage_preflight()==expected);assert(opened-o==closed-c);assert(before.size()==records.size());for(size_t i=0;i<before.size();i++)assert(before[i].key==records[i].key&&before[i].data==records[i].data&&before[i].type==records[i].type);printf("%s: PASS, all stored bytes unchanged\n",name);}
int main(){check(ESP_OK,"empty store");for(int i=1;i<=3;i++){add("our_sec_"+std::to_string(i));add("peer_sec_"+std::to_string(i));}check(ESP_OK,"three secure bonds");add("our_sec_4");check(ESP_ERR_NOT_SUPPORTED,"fourth previous bond blocked");records.pop_back();records[0].data.pop_back();check(ESP_ERR_INVALID_SIZE,"wrong blob layout blocked");records.clear();add("our_sec_1",false);check(ESP_ERR_NOT_SUPPORTED,"unauthenticated bond blocked");records.clear();add("peer_sec_1",true,false);check(ESP_ERR_NOT_SUPPORTED,"legacy bond blocked");records.clear();add("our_sec_1",true,true,7);check(ESP_ERR_NOT_SUPPORTED,"short key blocked");records.clear();add("our_sec_01");check(ESP_ERR_NOT_SUPPORTED,"malformed key blocked");records.clear();add("unknown_1");check(ESP_ERR_NOT_SUPPORTED,"unknown format preserved");records.clear();open_error=ESP_ERR_NVS_NOT_FOUND;check(ESP_OK,"fresh namespace");open_error=ESP_FAIL;check(ESP_FAIL,"read failure preserved");}
