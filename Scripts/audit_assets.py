"""Fail if any public asset has a dependency outside its demo, Engine or approved modules."""
from pathlib import Path
import json
import unreal
registry=unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
registry.scan_paths_synchronous(['/Game'], True)
assets=registry.get_assets_by_path('/Game',recursive=True)
packages={str(a.package_name) for a in assets}
options=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,
    include_searchable_names=False,include_soft_management_references=False,include_hard_management_references=False)
errors=[]
for package in sorted(packages):
    if not package.startswith('/Game/Demos/'): errors.append([package,'outside public demo folders'])
    for dep in (registry.get_dependencies(package,options) or []):
        dep=str(dep)
        if dep.startswith('/Game/') and dep not in packages: errors.append([package,dep])
        if any(x in dep for x in ['/Common/','/External/','/StoneWindowsModularPack/']): errors.append([package,dep])
maps=[str(a.package_name) for a in assets if str(a.asset_class_path.asset_name)=='World']
report={'asset_count':len(assets),'maps':sorted(maps),'invalid_dependencies':errors}
out=Path(unreal.Paths.project_saved_dir())/'Validation/asset_audit.json'
out.parent.mkdir(parents=True,exist_ok=True); out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
assert not errors,errors
assert len(maps)>=6, maps
unreal.log('PUBLIC_ASSET_AUDIT_PASSED '+str(len(assets))+' assets; '+str(len(maps))+' maps')
