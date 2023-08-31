import bpy

# Functions

def GetLayerCollections(current, layerCollections):
    for child in current.children:
        if not child.exclude and not child.hide_viewport:
            layerCollections.append(child)
        GetLayerCollections(child, layerCollections)

def FindMaterialOutput(mat, output):
    if not mat.use_nodes:
        return None
    
    for node in mat.node_tree.nodes:
        if node.type == "OUTPUT_MATERIAL":
            return node
    
def FindBSDFNode(mat, output):
    materialOutput = FindMaterialOutput(mat, output)
    if not materialOutput:
        output[mat.name]["Errors"].append("NO_MATERIAL_OUTPUT")
        return
    
    surfaceInput = materialOutput.inputs["Surface"]
    if not surfaceInput.is_linked:        
        output[mat.name]["Errors"].append("MATERIAL_OUTPUT_SURFACE_INPUT_NOT_CONNECTED")
        return
    
    surfaceLink = surfaceInput.links[0]
    if not surfaceLink.from_node.type == "BSDF_PRINCIPLED":
        output[mat.name]["Errors"].append("MATERIAL_OUTPUT_SURFACE_INPUT_NOT_BSDF")
        return
    
    return surfaceLink.from_node

def CheckMaterial(mat, output):
    output[mat.name] = {}
    output[mat.name]["Images"] = {}
    output[mat.name]["Errors"] = []
    
    BSDFNode = FindBSDFNode(mat, output)
    if not BSDFNode:
        return
    
    for inputName in checkInputNames:
        input = BSDFNode.inputs[inputName]
        safeInputName = inputName.upper().replace(' ', '_')
        
        if not input.is_linked:
            continue
        
        link = input.links[0]
        textureNode = link.from_node
        
        if inputName == "Normal":
            if not textureNode.type == "NORMAL_MAP":
                output[mat.name]["Errors"].append(f"BSDF_{safeInputName}_INPUT_NOT_NORMAL_MAP")
                continue
            
            if not textureNode.inputs["Color"].is_linked:
                output[mat.name]["Errors"].append(f"NORMAL_MAP_COLOR_INPUT_NOT_CONNECTED")
                continue
                
            textureNode = textureNode.inputs["Color"].links[0].from_node
            
        if not textureNode.type == "TEX_IMAGE":
            output[mat.name]["Errors"].append(f"BSDF_{safeInputName}_INPUT_NOT_TEXTURE")
            continue
        
        image = textureNode.image
        output[mat.name]["Images"][image.name] = {}
        output[mat.name]["Images"][image.name]["IsPacked"] = image.packed_file != None

def CheckMaterials(output):
    for mat in bpy.data.materials:
        if mat.name == "Dots Stroke":
            continue
        
        CheckMaterial(mat, output)

# Main
layerCollections = []
GetLayerCollections(bpy.context.view_layer.layer_collection, layerCollections)

collections = "C|"
for col in layerCollections:
    collections += col.name + ","
print(collections)

checkInputNames = { "Base Color", "Metallic", "Roughness", "Normal" }

materialOutput = {}
CheckMaterials(materialOutput)

hasPacked = False
for k,v in materialOutput.items():
    
    for k2,v2 in v["Images"].items():
        if v2["IsPacked"] == True:
            hasPacked = True
            break
    
    if len(v["Errors"]) > 0:
        warningString = "M|" + k + "|";
        for error in v["Errors"]:
            warningString += error + ","
        print (warningString)

if hasPacked:
    print ("P|True")

print ("Analysis Complete")

if not bpy.app.background:
    bpy.ops.wm.quit_blender()