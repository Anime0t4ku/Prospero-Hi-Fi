// Run with NODE_PATH pointing to jsdom's parent node_modules directory.
const {JSDOM}=require('jsdom'),fs=require('node:fs'),assert=require('node:assert/strict');
const html=fs.readFileSync('include/web_ui.hpp','utf8').split('R"NXWEB(')[1].split(')NXWEB"')[0];
const state=JSON.parse(fs.readFileSync(process.env.PROSPERO_WEB_STATE,'utf8')),calls=[],errors=[];
const dom=new JSDOM(html,{url:'http://ps5:8182',runScripts:'dangerously',pretendToBeVisual:true,beforeParse(w){
 w.requestAnimationFrame=()=>0;w.scrollTo=()=>{};w.HTMLCanvasElement.prototype.getContext=()=>({clearRect(){},fillRect(){}});w.HTMLElement.prototype.animate=()=>({cancel(){}});
 w.WebSocket=class{constructor(){this.close=()=>{};}};w.fetch=async(url,opt)=>{
  if(url==='/api/cmd'){const cmd=JSON.parse(opt.body);calls.push(cmd);if(cmd.op==='listening_mode')state.listening_mode=cmd.enabled;if(cmd.op==='settings')Object.assign(state.settings,cmd.patch);if(cmd.op==='playpause')state.playback.paused=!state.playback.paused;return {ok:true,json:async()=>({ok:true,revision:++state.revision})};}
  if(url==='/api/browse')return {ok:true,json:async()=>({source:-1,path:'',sources_revision:1,entries:[{name:'Track 1.flac',path:'Track 1.flac'},{name:'Track 2.flac',path:'Track 2.flac'}]})};
  return {ok:true,json:async()=>structuredClone(state)};
 };w.addEventListener('error',e=>errors.push(e.message));}});
const w=dom.window,d=w.document,$=id=>d.getElementById(id),settle=()=>new Promise(r=>setTimeout(r,30));
(async()=>{await settle();assert.equal($('connection').textContent,'Prospero Hi-Fi · v'+state.version+' · Connected');assert.equal($('time').textContent,'0:25');assert.equal($('duration').textContent,'2:00');
 assert.deepEqual([...$('nav').children].map(b=>b.textContent),['Browse','Player','Settings']);
 $('nav').children[2].click();[...d.querySelectorAll('.settingsCategories button')].find(b=>b.textContent==='Visualizer').click();
 const canvas=$('spectrum'),styles=d.querySelector('[data-key="visualizer_style"]');assert.equal(styles.options.length,11);assert.equal(styles.options[0].textContent,'Stock spectrum');assert(![...styles.options].some(o=>o.textContent.includes('Radial')));
 styles.value='18';styles.dispatchEvent(new w.Event('change'));await settle();assert(calls.some(c=>c.patch?.visualizer_style===18));assert.equal($('spectrum'),canvas);
 [...d.querySelectorAll('.settingsCategories button')].find(b=>b.textContent==='Equalizer').click();assert.equal(d.querySelectorAll('input[type=range][data-key^="eq_"]').length,5);
 const gain=d.querySelector('[data-key="eq_0"]');gain.value='3.5';gain.dispatchEvent(new w.Event('change'));await settle();assert(calls.some(c=>c.patch?.eq_0===3.5));
 $('nav').children[0].click();
 const children=[...$('player').children];for(const [a,b] of [['spectrum','progress'],['progress','volume'],['volume','transport']])assert(children.indexOf(a==='volume'?$(a).closest('.volume'):$(a))<children.indexOf(b==='volume'?$(b).closest('.volume'):$(b)));
 $('nav').children[2].click();[...d.querySelectorAll('.settingsCategories button')].find(b=>b.textContent==='Listening Mode').click();assert.equal(d.querySelectorAll('.settingsEntries input[type=checkbox]').length,4);
 const setting=d.querySelector('.settingsEntries input');setting.checked=false;setting.dispatchEvent(new w.Event('change'));await settle();assert.equal(state.settings.listening_hide_title,false);
 $('nav').children[1].click();assert(d.body.classList.contains('mobilePlayer'));$('listening-toggle').click();await settle();assert(state.listening_mode);$('ctl-playpause').click();await settle();assert(state.listening_mode);$('listening-toggle').click();await settle();assert(!state.listening_mode);
 $('nav').children[0].click();assert(!d.body.classList.contains('mobilePlayer'));
 assert.equal(d.querySelectorAll('.sourceRow').length,1);assert(d.querySelector('.sourceEditor').hidden);
 const edit=d.querySelector('.sourceAction');edit.click();assert(!d.querySelector('.sourceEditor').hidden);assert.equal(edit.getAttribute('aria-expanded'),'true');
 const folder=d.querySelector('.sourceEditor input');folder.value='/data/Albums';[...d.querySelectorAll('.sourceEditor button')].find(b=>b.textContent==='Save').click();await settle();assert.equal(state.settings.music_root,'/data/Albums');assert(d.querySelector('.sourceEditor').hidden);assert.equal(d.querySelector('.sourceDetail').textContent,'/data/Albums');
 edit.click();folder.value='/data/Cancelled';[...d.querySelectorAll('.sourceEditor button')].find(b=>b.textContent==='Cancel').click();assert(d.querySelector('.sourceEditor').hidden);assert.equal(state.settings.music_root,'/data/Albums');
 [...d.querySelectorAll('#browser button')].find(b=>b.getAttribute('aria-label')==='Open Internal Storage').click();await settle();const filter=d.querySelector('input[type=search]');filter.value='Track 2';filter.dispatchEvent(new w.Event('input'));assert.equal([...d.querySelectorAll('.list .entry')].filter(e=>!e.hidden).length,1);
 const progress=$('progress');progress.onpointerdown();progress.value=800;await w.eval('poll()');assert.equal(progress.value,'800');progress.onchange();await settle();assert(calls.some(c=>c.op==='seek_to'&&c.position===96));
 $('ctl-fullscreen').click();assert(d.body.classList.contains('fs'));$('nav').children[0].click();assert(!d.body.classList.contains('fs'));assert.equal(d.querySelector('input[type=search]').value,'Track 2');
 assert.deepEqual(errors,[]);console.log('Web DOM: bootstrap, dynamic sources, grouped settings, Listening Mode, playback retention, filtering, view navigation, duration placement and seek drag passed.');dom.window.close();
})().catch(e=>{console.error(e);dom.window.close();process.exitCode=1;});
