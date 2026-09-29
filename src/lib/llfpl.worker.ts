import { executeLlfpl } from './llfpl';
self.onmessage = (event: MessageEvent<string>) => {
  self.postMessage(executeLlfpl(event.data));
};
