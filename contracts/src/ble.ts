export const BLE_UUIDS={service:'a90d0001-6d64-4f43-9e6d-9db51f1f0000',metadata:'a90d0002-6d64-4f43-9e6d-9db51f1f0000',input:'a90d0003-6d64-4f43-9e6d-9db51f1f0000',output:'a90d0004-6d64-4f43-9e6d-9db51f1f0000'} as const;
export function fragmentMessage(bytes:Uint8Array,transferId:number,mtu=23):Uint8Array[] {
 if(bytes.length<1||bytes.length>2048||!Number.isInteger(mtu)||mtu<23||mtu>517||!Number.isInteger(transferId)||transferId<0||transferId>0xffffffff) throw new Error('invalid_fragment');
 const size=mtu-11, result:Uint8Array[]=[];
 for(let offset=0;offset<bytes.length;offset+=size){
  const data=bytes.subarray(offset,offset+size), packet=new Uint8Array(8+data.length), view=new DataView(packet.buffer);
  view.setUint32(0,transferId,true);view.setUint16(4,offset,true);view.setUint16(6,bytes.length,true);packet.set(data,8);result.push(packet);
 }return result;
}
export class BleAssembler {
 private active?:{id:number;total:number;offset:number;start:number;bytes:Uint8Array};
 reset(){this.active=undefined;}
 push(packet:Uint8Array,nowMs:number):Uint8Array|null {
  const fail=():never=>{this.reset();throw new Error('invalid_fragment');};
  if(packet.length<=8||packet.length>514||!Number.isFinite(nowMs)) return fail();
  const view=new DataView(packet.buffer,packet.byteOffset,packet.byteLength), id=view.getUint32(0,true),offset=view.getUint16(4,true),total=view.getUint16(6,true),data=packet.subarray(8);
  if(total<1||total>2048||offset+data.length>total) return fail();
  if(this.active&&(nowMs-this.active.start>=10000||nowMs<this.active.start)) return fail();
  if(!this.active){if(offset!==0)return fail();this.active={id,total,offset:0,start:nowMs,bytes:new Uint8Array(total)};}
  const a=this.active;
  if(a.id!==id||a.total!==total||a.offset!==offset)return fail();
  a.bytes.set(data,offset);a.offset+=data.length;
  if(a.offset===a.total){this.reset();return a.bytes;}
  return null;
 }
}
