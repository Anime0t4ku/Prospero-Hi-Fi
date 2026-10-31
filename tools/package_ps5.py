#!/usr/bin/env python3
"""Assemble a complete ShadowMount+ native title folder."""
from pathlib import Path
import shutil,json,hashlib
root=Path(__file__).resolve().parents[1]
app=root/'dist/PPSA99287';(app/'sce_sys').mkdir(parents=True,exist_ok=True)
shutil.copytree(root/'assets',app/'assets',dirs_exist_ok=True)
shutil.copy(root/'build-ps5/access-helper.elf',app/'assets/access-helper.elf')
param={"ageLevel":{"default":0},"applicationCategoryType":0,"applicationDrmType":"free","attribute":0,"attribute2":0,"attribute3":0,
"conceptId":"99287","contentBadgeType":1,"contentId":"UP9000-PPSA99287_00-PROSPEROHIFI0001","contentVersion":"01.030.000","downloadDataSize":0,
"localizedParameters":{"defaultLanguage":"en-US","en-US":{"titleName":"Prospero Hi-Fi"}},"masterVersion":"01.01",
"requiredSystemSoftwareVersion":"0x0000000000000000","sdkVersion":"0x0000000000000000","titleId":"PPSA99287","versionFileUri":""}
(app/'sce_sys/param.json').write_text(json.dumps(param,indent=2)+'\n')
for name in ['icon0.png','pic1.png']:
    source=root/'assets'/name
    if not source.is_file():raise SystemExit('Missing '+str(source))
    shutil.copy(source,app/'sce_sys'/name)
shutil.copy(root/'README.md',root/'dist/INSTALL.txt')
shutil.copy(root/'BUILD_PROVENANCE.json',app/'BUILD_PROVENANCE.json')
shutil.copy(root/'THIRD_PARTY_NOTICES.md',app/'NOTICE.txt')
shutil.copy(root/'VALIDATION.md',root/'dist/VALIDATION.txt')
shutil.copytree(root/'licenses',app/'licenses',dirs_exist_ok=True)
shutil.copy(root/'LICENSE',app/'licenses/Prospero-GPL-3.0.txt')
files={str(p.relative_to(app)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(app.rglob('*')) if p.is_file() and p.name!='SHA256SUMS.json'}
(app/'SHA256SUMS.json').write_text(json.dumps(files,indent=2)+'\n')
print('Assembled',app,'with',len(files),'files')
