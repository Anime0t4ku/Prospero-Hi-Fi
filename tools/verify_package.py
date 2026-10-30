#!/usr/bin/env python3
from pathlib import Path
import json,hashlib,struct,re
root=Path(__file__).resolve().parents[1];app=root/'dist/PPSA99287'
required=['eboot.bin','sce_module/libc.prx','sce_sys/param.json','sce_sys/icon0.png','sce_sys/pic1.png','assets/font.ttf','assets/cacert.pem','assets/access-helper.elf']
for name in required:assert (app/name).is_file(),name
param=json.loads((app/'sce_sys/param.json').read_text())
assert param['titleId']==app.name
assert re.fullmatch(r'[A-Z0-9]{6}-[A-Z]{4}[0-9]{5}_00-[A-Z0-9]{16}',param['contentId'])
assert param['localizedParameters']['en-US']['titleName']=='Prospero Hi-Fi'
assert (app/'eboot.bin').read_bytes()[:4]==bytes.fromhex('4f153d1d')
assert (app/'sce_module/libc.prx').read_bytes()[:4]==bytes.fromhex('5414f5ee')
assert hashlib.sha256((app/'sce_module/libc.prx').read_bytes()).hexdigest()=='e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036'
assert (app/'assets/access-helper.elf').read_bytes()[:4]==b'\x7fELF'
for name,dims in [('icon0.png',(512,512)),('pic1.png',(1920,1080))]:
    assert struct.unpack('>II',(app/'sce_sys'/name).read_bytes()[16:24])==dims
for name,digest in json.loads((app/'SHA256SUMS.json').read_text()).items():
    assert hashlib.sha256((app/name).read_bytes()).hexdigest()==digest,name
print('Native title folder, FSELF signatures, runtime digest, metadata, images, helper and asset checksums verified.')
