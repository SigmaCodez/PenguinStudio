# SPDX-License-Identifier: GPL-2.0-or-later
from pathlib import Path
import shutil,subprocess,tempfile
repo=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='penguin-websocket-patch-') as d:
 root=Path(d); target=root/'plugins/obs-websocket'
 for name in ['src/eventhandler/EventHandler.cpp','src/eventhandler/EventHandler.h']:
  dst=target/name;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(subprocess.check_output(['git','-C',str(repo/'plugins/obs-websocket'),'show',f'HEAD:{name}']))
 wrapper=root/'verify.cmake';wrapper.write_text(f'set(CMAKE_SOURCE_DIR "{repo}")\nset(CMAKE_CURRENT_SOURCE_DIR "{root}/plugins")\ninclude("{repo}/cmake/common/penguin_websocket_patch.cmake")\n')
 command=['cmake','-P',str(wrapper)]
 subprocess.run(command,check=True,capture_output=True)
 first=(target/'src/eventhandler/EventHandler.cpp').read_bytes()
 assert b'case OBS_FRONTEND_EVENT_EXIT:' in first
 subprocess.run(command,check=True,capture_output=True)
 assert (target/'src/eventhandler/EventHandler.cpp').read_bytes()==first
 (target/'src/eventhandler/EventHandler.cpp').write_text('incompatible upstream source\n')
 result=subprocess.run(command,capture_output=True,text=True)
 assert result.returncode!=0 and 'Review the Penguin WebSocket' in result.stderr
 print('PASS: patch applies, is idempotent, and rejects incompatible source')
