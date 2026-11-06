"""Optional visual smoke check. Launch a separate editor with this ExecutePythonScript.
Starts PIE, captures Saved/Screenshots/GrassDemo.png, ends PIE, and exits that editor.
"""
from pathlib import Path
import time
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level("/Game/Demos/Grass/Maps/L_GrassInteraction")
started = time.monotonic()
state = {"stage": 0, "play_started": None, "task": None, "frames": 0}
output = Path(unreal.Paths.project_saved_dir()).resolve() / "Screenshots/GrassDemo.png"
output.parent.mkdir(parents=True, exist_ok=True)


def tick(delta_time):
    now = time.monotonic()
    if now - started > 180:
        unreal.log_error("GRASS_CAPTURE_TIMEOUT")
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
        return
    if state["stage"] == 0 and now - started > 3:
        levels.editor_request_begin_play()
        state["stage"] = 1
    elif state["stage"] == 1 and levels.is_in_play_in_editor():
        state["play_started"] = now
        state["stage"] = 2
    elif state["stage"] == 2:
        state["frames"] += 1
        if state["frames"] > 180 and now - state["play_started"] > 4:
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            demos = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.GrassInteractionDemo)
            assert demos and demos[0].get_editor_property("current_state"), "PIE simulation was not initialized"
            unreal.log("GRASS_PIE_INITIALIZED: RT=" + demos[0].get_editor_property("current_state").get_name())
            # Screenshot preparation can pump Slate ticks; set the stage before calling it.
            state["stage"] = 3
            state["task"] = unreal.AutomationLibrary.take_high_res_screenshot(1280, 960, str(output))
    elif state["stage"] == 3 and state["task"] and state["task"].is_task_done():
        unreal.log("GRASS_CAPTURE_SAVED: " + str(output))
        state["stage"] = 4
        levels.editor_request_end_play()
    elif state["stage"] == 4 and not levels.is_in_play_in_editor():
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)
