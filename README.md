# What is this?
This repository contains Unreal Engine plugin with functionality to extract and browse Divinity 2: Ego Draconis assets. I created this plugin because I wanted to try to move this game to UE. Regardless of the success of my goal, you can use it for viewing game assets and probably modding (with few adjustments).

# Setup
1. Create empty UE project, this is required for UE to have some workspace.
2. Clone this repo into Plugins folder.
3. Open project in UE.
4. Make sure you see `Divinity 2` menu entry in top bar. If not, open `Edit` > `Plugins` and make sure DV2 plugin is **enabled**.
5. Open `Edit` > `Project Settings`, navigate to `Engine` > `Divinity 2` section. Make sure settings are set up:

| Setting                     | Description                                                                                                                                                                                              |
|-----------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Divinity 2 Asset Location` | Unpacked assets location within installed game directory<br>Can be found under `<Game Install Dir>/Data/Win32/Packed`<br>Example: `D:\SteamLibrary\steamapps\common\divinity2_dev_cut\Data\Win32\Packed` |
| `Export Location`           | Where to extract files when required<br>I would recommend to set it to `<Project Dir>/Content/DV2`                                                                                                       |
| `Base Material`             | Material to apply when openning NIF scenes<br>Should be `M_DV2Basic` material in plugin content dir                                                                                                      |
| `Nif Meta Files`            | Metadata files for reading NIF scenes<br>Should point to `<Project Dir>/Plugins/DV2/Resources/NiMeta/nif.xml`                                                                                            |

6. When you change settings, you should reload caches:
    - `Divinity 2` > `Reload DV2 Assets` - reload asset list cache, should be done whenever you change `Divinity 2 Asset Location`
    - `Divinity 2` > `Reload NIF Metadata` - reload metadata for nif nodes, should be done whenever you change paths in `Nif Meta Files`

_Metadata is reloaded any time you open the project, you need to reload it manually only when you change settings._
7. Now you can open `Divinity 2` > `DV2 Browser`, you can list assets and view them here. Asset file formats supported are listed in separate sectiopn.

## Supported asset formats
| Format | Description                                                                                            |
|--------|--------------------------------------------------------------------------------------------------------|
| `.lua` | lua scripts                                                                                            |
| `.nif` | NI block trees, representing scenes                                                                    |
| `.dds` | single-block NI trees, single block is `NiPersistentSrcTextureRenderData`, NI block with texture data  |
| `.xml` | single-block NI trees, single block is `xml::dom::CStreamableNode`, custom Larian format for tree data |
| `.kf`  | partial support, NI trees representing animations                                                      |

# Blueprints
## DV2 Ghost
Component/Actor to display meshes from diivinity 2 assets on UE level

## DV2AssetPath meta-tag
Use this on `FString` `UPRROPERTY` and `UPARAM` to make fields and pins display specific widget for path selection. Configuration is provided in value via comma-separated list of keys.
- `dir` - allow selection of directories
- `file` - allow selection of files of any format

Example: directory-only selection
```
UPROPERTY(meta=(DV2AssetPath="dir"))
FString SomeProperty;

UFUNCTION(BlueprintCallable)
void SomeFunc(UPARAM(meta=(DV2AssetPath="dir")) const FString& SomeParam);
```

# Dictionary
- DV2 - divinity asset as is on drive, kind of larian-specific zip archieve
- NI - Any Net-Immerse asset, represent set of blocks that form data tree
- NIF - Asset file format that stores NI tree, representing scene
- XML - Asset file format that stores NI tree with one node, this node stores table and this node type seem to be unique
  larian type