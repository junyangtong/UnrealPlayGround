"""Run in a separate offscreen editor. Capture real PIE frames, then exit that editor."""
from pathlib import Path
import time
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
root=Path(unreal.Paths.project_dir()).resolve()
output=root/'Docs/Images'; output.mkdir(parents=True,exist_ok=True)
jobs=[('/Game/Demos/Bartending/Maps/L_Bartending','Bartending'),
      ('/Game/Demos/StainedGlass/Maps/L_StainedGlass','StainedGlass'),
      ('/Game/Demos/Grass/Maps/L_GrassInteraction','Grass'),
      ('/Game/Demos/WFC/Maps/L_WFC','WFC'),
      ('/Game/Demos/WFC/MarchingSquares/L_SquareModelGallery','MarchingSquares')]
state={'index':0,'stage':'load','at':time.monotonic(),'task':None,'frames':0}
start=time.monotonic()
def tick(dt):
    try:
        now=time.monotonic()
        if now-start>420: raise RuntimeError('Capture timed out')
        stage=state['stage']
        if stage=='load':
            if state['index']>=len(jobs):
                unreal.log('PUBLIC_CAPTURES_COMPLETE')
                unreal.unregister_slate_post_tick_callback(handle); unreal.SystemLibrary.quit_editor(); return
            path,name=jobs[state['index']]
            state['stage']='start'
            assert levels.load_level(path),path
            state['at']=now
        elif stage=='start' and now-state['at']>3:
            state['stage']='play'; levels.editor_request_begin_play()
        elif stage=='play' and levels.is_in_play_in_editor():
            world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            name=jobs[state['index']][1]
            if name=='WFC':
                grid=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.WFCGridGenerator)[0]
                grid.generate()
            state.update(stage='wait',at=now,frames=0)
        elif stage=='wait':
            state['frames']+=1
            if now-state['at']>6 and state['frames']>90:
                state['stage']='capture'; state['requested']=time.time(); state['at']=now
                state['task']=unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,str(output/(jobs[state['index']][1]+'.png')))
        elif stage=='capture' and (output/(jobs[state['index']][1]+'.png')).exists() and (output/(jobs[state['index']][1]+'.png')).stat().st_mtime>state['requested'] and now-state['at']>4:
            unreal.log('PUBLIC_CAPTURE_SAVED '+jobs[state['index']][1])
            state['stage']='stop'; levels.editor_request_end_play()
        elif stage=='stop' and not levels.is_in_play_in_editor():
            state['index']+=1; state['stage']='load'
    except Exception as e:
        unreal.log_error('PUBLIC_CAPTURE_FAILED '+repr(e))
        unreal.unregister_slate_post_tick_callback(handle); unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
