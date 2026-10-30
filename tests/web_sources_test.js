const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const header=fs.readFileSync('include/web_ui.hpp','utf8');
const script=header.split('<script>')[1].split('</script>')[0];new vm.Script(script); // Parse the complete embedded web client.
const sourceFn=script.slice(script.indexOf('function renderSources()'),script.indexOf('function renderEQ()'));
const make=(tag,text,cls)=>({tag,text,cls,style:{},children:[],append(...nodes){this.children.push(...nodes)},replaceChildren(...nodes){this.children=nodes}});
const root=make('main');let opened,saved;
const context={state:{revision:7,sources_revision:9,settings:{music_root:'/data/music',storage_folders:{'/mnt/usb7':'/mnt/usb7/Albums'},shares:[]},local_sources:[{id:-1,name:'Internal Storage',mount:'/data',folder:'/data/music'}]},shareOpen:false,
$:()=>root,E:make,field:(label,type,value)=>({wrap:make('label',label),input:{value}}),button:(text,action)=>({...make('button',text),action}),browse:(...args)=>opened=args,patch:(...args)=>saved=args};
vm.createContext(context);vm.runInContext(sourceFn,context);context.renderSources();
const cards=()=>root.children.filter(n=>n.cls==='card');assert.equal(cards().length,1);assert.equal(cards()[0].children[0].text,'Internal Storage');
context.state.local_sources.push({id:-3,name:'M.2 Storage',mount:'/mnt/ext1',folder:'/mnt/ext1/Music'},{id:-10,name:'USB Storage 1',mount:'/mnt/usb0',folder:'/mnt/usb0/Music'},{id:-2,name:'Physical Disc',mount:'cdda:',folder:'cdda:'});
context.renderSources();assert.equal(cards().length,4);
cards()[1].children[2].children[0].action();assert.deepEqual(opened,[-3,'']);
cards()[2].children[2].children[1].action();assert.equal(saved[0].storage_folders['/mnt/usb0'],'/mnt/usb0/Music');assert.equal(saved[0].storage_folders['/mnt/usb7'],'/mnt/usb7/Albums');assert.equal(saved[1],7);
cards()[3].children[2].action();assert.deepEqual(opened,[-2,'']);
context.state.local_sources=context.state.local_sources.slice(0,1);context.renderSources();assert.equal(cards().length,1);
console.log('Web source visibility, drive IDs, folder patches and disc action passed; complete client JavaScript parses.');
