import { executeLlfpl, type LlfplModule } from './llfpl';
self.onmessage = async (event: MessageEvent<string>) => {
  try {
    const url=new URL('/wasm/llfpl.mjs',self.location.origin).href;
    const {default:factory}=await import(/* @vite-ignore */ url);
    const module:LlfplModule=await factory({locateFile:(file:string)=>new URL(`/wasm/${file}`,self.location.origin).href});
    self.postMessage({type:'ready'});
    self.postMessage({type:'result',result:executeLlfpl(module,event.data)});
  } catch(error) {
    self.postMessage({type:'error',message:error instanceof Error?error.message:String(error)});
  }
};
