#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Map.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Enemy.h"
#include "hoguera.h"

#include <math.h>
#include <algorithm>

Map::Map() : Module(), mapLoaded(false), helpTexture(nullptr), showHelpTexture(false)
{
    name = "map";
}

// Destructor
Map::~Map()
{}

// Called before render is available
bool Map::Awake()
{
    name = "map";
    LOG("Loading Map Parser");

    return true;
}

bool Map::Start() {

    helpTexture = Engine::GetInstance().textures->Load("Assets/Textures/help.png"); 
    showHelpTexture = false;
    
    float fw = 0, fh = 0;
	if (SDL_GetTextureSize(helpTexture, &fw, &fh)) {
		helpTextureRect.w = (int)fw;
		helpTextureRect.h = (int)fh;
		helpTextureRect.x = Engine::GetInstance().render->camera.w - (int)fw - 10; 
		helpTextureRect.y = 10; 
	}
    
    return true;
}

bool Map::Update(float dt)
{
    bool ret = true;


    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_H) == KEY_DOWN) {
        showHelpTexture = !showHelpTexture;
    }

    if (mapLoaded) {

        // iterar todas las capas del mapa
        for (const auto& mapLayer : mapData.layers) {

            // NUEVO CÓDIGO:
            Properties::Property* drawProp = mapLayer->properties.GetProperty("Draw");

            // si no hay propiedad "Draw", o es true ? dibuja
            if (drawProp == nullptr || drawProp->value == true)
            {
                for (int i = 0; i < mapData.height; i++) {
                    for (int j = 0; j < mapData.width; j++) {

                        int gid = mapLayer->Get(i, j);

                        if (gid != 0) {

                            TileSet* tileSet = GetTilesetFromTileId(gid);
                            if (tileSet != nullptr) {

                                SDL_Rect tileRect = tileSet->GetRect(gid);
                                Vector2D mapCoord = MapToWorld(i, j);

                                Engine::GetInstance().render->DrawTexture(
                                    tileSet->texture,
                                    (int)mapCoord.getX(),
                                    (int)mapCoord.getY(),
                                    &tileRect
                                );
                            }
                        }
                    }
                }
            }
        }

        if (showHelpTexture && helpTexture != nullptr) {
            Engine::GetInstance().render->DrawTexture(helpTexture, helpTextureRect.x, helpTextureRect.y);
        }
    }

    return ret;
}

// L09: TODO 2: Implement function to the Tileset based on a tile id
TileSet* Map::GetTilesetFromTileId(int gid) const
{
	TileSet* set = nullptr;
    for(const auto& tileset : mapData.tilesets) {
        set = tileset;
        if (gid >= tileset->firstGid && gid < tileset->firstGid + tileset->tileCount) {
			break;
        }
	}
    return set;
}

// Called before quitting
bool Map::CleanUp()
{
    LOG("Unloading map");


    if (helpTexture != nullptr) {
        Engine::GetInstance().textures->UnLoad(helpTexture);
        helpTexture = nullptr;
    }

    // L06: TODO 2: Make sure you clean up any memory allocated from tilesets/map
    for (const auto& tileset : mapData.tilesets) {
        delete tileset;
    }
    mapData.tilesets.clear();

    // L07 TODO 2: clean up all layer data
    for (const auto& layer : mapData.layers)
    {
        delete layer;
    }
    mapData.layers.clear();

    return true;
}

// Load new map
bool Map::Load(std::string path, std::string fileName)
{
    bool ret = false;

    // Assigns the name of the map file and the path
    mapFileName = fileName;
    mapPath = path;
    std::string mapPathName = mapPath + mapFileName;

    pugi::xml_document mapFileXML;
    pugi::xml_parse_result result = mapFileXML.load_file(mapPathName.c_str());

    if(result == NULL)
	{
		LOG("Could not load map xml file %s. pugi error: %s", mapPathName.c_str(), result.description());
		ret = false;
    }
    else {

        // L06: TODO 3: Implement LoadMap to load the map properties
        // retrieve the paremeters of the <map> node and store the into the mapData struct
        mapData.width = mapFileXML.child("map").attribute("width").as_int();
        mapData.height = mapFileXML.child("map").attribute("height").as_int();
        mapData.tileWidth = mapFileXML.child("map").attribute("tilewidth").as_int();
        mapData.tileHeight = mapFileXML.child("map").attribute("tileheight").as_int();

        // L06: TODO 4: Implement the LoadTileSet function to load the tileset properties
       
        //Iterate the Tileset
        for(pugi::xml_node tilesetNode = mapFileXML.child("map").child("tileset"); tilesetNode!=NULL; tilesetNode = tilesetNode.next_sibling("tileset"))
		{
            //Load Tileset attributes
			TileSet* tileSet = new TileSet();
            tileSet->firstGid = tilesetNode.attribute("firstgid").as_int();
            tileSet->name = tilesetNode.attribute("name").as_string();
            tileSet->tileWidth = tilesetNode.attribute("tilewidth").as_int();
            tileSet->tileHeight = tilesetNode.attribute("tileheight").as_int();
            tileSet->spacing = tilesetNode.attribute("spacing").as_int();
            tileSet->margin = tilesetNode.attribute("margin").as_int();
            tileSet->tileCount = tilesetNode.attribute("tilecount").as_int();
            tileSet->columns = tilesetNode.attribute("columns").as_int();

			// Nos dice exacto qué tileset se ha cargado (PRUEBA)
            LOG("Tileset cargado: %s  firstgid=%d  tileCount=%d columns=%d",
                tileSet->name.c_str(), tileSet->firstGid, tileSet->tileCount, tileSet->columns);

			//Load the tileset image
			std::string imgName = tilesetNode.child("image").attribute("source").as_string();
            tileSet->texture = Engine::GetInstance().textures->Load((mapPath+imgName).c_str());

			mapData.tilesets.push_back(tileSet);
		}

        // L07: TODO 3: Iterate all layers in the TMX and load each of them
        for (pugi::xml_node layerNode = mapFileXML.child("map").child("layer"); layerNode != NULL; layerNode = layerNode.next_sibling("layer")) {

            // L07: TODO 4: Implement the load of a single layer 
            //Load the attributes and saved in a new MapLayer
            MapLayer* mapLayer = new MapLayer();
            mapLayer->id = layerNode.attribute("id").as_int();
            mapLayer->name = layerNode.attribute("name").as_string();
            mapLayer->width = layerNode.attribute("width").as_int();
            mapLayer->height = layerNode.attribute("height").as_int();

            //L09: TODO 6 Call Load Layer Properties
            LoadProperties(layerNode, mapLayer->properties);

            //Iterate over all the tiles and assign the values in the data array
            for (pugi::xml_node tileNode = layerNode.child("data").child("tile"); tileNode != NULL; tileNode = tileNode.next_sibling("tile")) {
                mapLayer->tiles.push_back(tileNode.attribute("gid").as_int());
            }

            //add the layer to the map
            mapData.layers.push_back(mapLayer);
        }

        // L08 TODO 3: Create colliders
        // L08 TODO 7: Assign collider type
        // Later you can create a function here to load and create the colliders from the map

        //Iterate the layer and create colliders
      // === COLISIONES DEL MAPA ===
       // === COLISIONES DEL MAPA ===
       // === COLISIONES DEL MAPA ===
        for (const auto& mapLayer : mapData.layers)
        {
            if (mapLayer->name == "Collisions")
            {
                for (int i = 0; i < mapData.height; i++)
                {
                    for (int j = 0; j < mapData.width; j++)
                    {
                        int gid = mapLayer->Get(i, j);
                        if (gid == 0) continue;

                        Vector2D pos = MapToWorld(i, j);

                        // --- Creamos collider físico ---
                        PhysBody* c = Engine::GetInstance().physics->CreateRectangle(
                            pos.getX() + mapData.tileWidth / 2,
                            pos.getY() + mapData.tileHeight / 2,
                            mapData.tileWidth, mapData.tileHeight,
                            STATIC);

                        // 1) Averiguamos de qué tileset viene este gid
                        TileSet* ts = GetTilesetFromTileId(gid);
                        if (ts == nullptr)
                        {
                            c->ctype = ColliderType::UNKNOWN;
                            continue;
                        }

                        // 2) Si NO es MapMetadata, lo tratamos como suelo por defecto
                        if (ts->name != "MapMetadata")
                        {
                            c->ctype = ColliderType::PLATFORM;
                            continue;
                        }

                        // 3) Si SÍ es MapMetadata, calculamos el índice local (0,1,2...)
                        int localId = gid - ts->firstGid;

                        // 4) Asignamos tipo según el color
                        switch (localId)
                        {
                        case 0: // rojo
                            c->ctype = ColliderType::PLATFORM;
                            break;
                        case 1: // verde
                            c->ctype = ColliderType::TOPE;
                            break;
                        case 2: // azul
                            c->ctype = ColliderType::WALL;
                            break;
                        default:
                            c->ctype = ColliderType::UNKNOWN;
                            break;
                        }

                        // DEBUG opcional:
                        LOG("COLL TILE gid=%d  firstGid=%d  local=%d  type=%d",
                            gid, ts->firstGid, localId, (int)c->ctype);
                    }
                }
            }
        }

        for (pugi::xml_node objGroup = mapFileXML.child("map").child("objectgroup");
            objGroup;
            objGroup = objGroup.next_sibling("objectgroup"))
        {
            std::string layerName = objGroup.attribute("name").as_string();

            if (layerName == "Entities")   // nombre de la capa de objetos en Tiled
            {
                for (pugi::xml_node object = objGroup.child("object");
                    object;
                    object = object.next_sibling("object"))
                {
                    std::string type = object.attribute("type").as_string();
                    std::string name = object.attribute("name").as_string(); 
                    float x = object.attribute("x").as_float();
                    float y = object.attribute("y").as_float();

                    if (name == "Slime")  
                    {
                        auto e = Engine::GetInstance().entityManager->CreateEntity(EntityType::ENEMY);
                        if (e)
                        {
                            std::shared_ptr<Enemy> slime = std::dynamic_pointer_cast<Enemy>(e);
                            if (slime)
                            {
                                slime->SetPosition((int)x, (int)y);
                            }
                        }
                    }
                    else if (name == "hoguera")
                    {
                        auto h = std::make_shared<hoguera>((int)x, (int)y);
                        Engine::GetInstance().entityManager->AddEntity(h);
                    }
                }
            }
        }

        ret = true;

        // L06: TODO 5: LOG all the data loaded iterate all tilesetsand LOG everything
        if (ret == true)
        {
            LOG("Successfully parsed map XML file :%s", fileName.c_str());
            LOG("width : %d height : %d", mapData.width, mapData.height);
            LOG("tile_width : %d tile_height : %d", mapData.tileWidth, mapData.tileHeight);
            LOG("Tilesets----");

            //iterate the tilesets
            for (const auto& tileset : mapData.tilesets) {
                LOG("name : %s firstgid : %d", tileset->name.c_str(), tileset->firstGid);
                LOG("tile width : %d tile height : %d", tileset->tileWidth, tileset->tileHeight);
                LOG("spacing : %d margin : %d", tileset->spacing, tileset->margin);
            }
            			
            LOG("Layers----");

            for (const auto& layer : mapData.layers) {
                LOG("id : %d name : %s", layer->id, layer->name.c_str());
				LOG("Layer width : %d Layer height : %d", layer->width, layer->height);
            }   
        }
        else {
            LOG("Error while parsing map file: %s", mapPathName.c_str());
        }

        if (mapFileXML) mapFileXML.reset();

    }

    mapLoaded = ret;
    return ret;
}

// L07: TODO 8: Create a method that translates x,y coordinates from map positions to world positions
Vector2D Map::MapToWorld(int i, int j) const
{
    Vector2D ret;

    ret.setX((float)(j * mapData.tileWidth));
    ret.setY((float)(i * mapData.tileHeight));

    return ret;
}

// L09: TODO 6: Load a group of properties from a node and fill a list with it
bool Map::LoadProperties(pugi::xml_node& node, Properties& properties)
{
    bool ret = false;

    for (pugi::xml_node propertieNode = node.child("properties").child("property"); propertieNode; propertieNode = propertieNode.next_sibling("property"))
    {
        Properties::Property* p = new Properties::Property();
        p->name = propertieNode.attribute("name").as_string();
        p->value = propertieNode.attribute("value").as_bool(); 

        properties.propertyList.push_back(p);
    }

    return ret;
}

// L10: TODO 7: Create a method to get the map size in pixels
Vector2D Map::GetMapSizeInPixels()
{
    Vector2D sizeInPixels;
    sizeInPixels.setX((float)(mapData.width * mapData.tileWidth));
    sizeInPixels.setY((float)(mapData.height * mapData.tileHeight));
    return sizeInPixels;
}

void Map::WorldToMap(int x, int y, int& row, int& col) const
{
    if (mapData.tileWidth <= 0 || mapData.tileHeight <= 0) {
        row = col = 0;
        return;
    }

    col = x / mapData.tileWidth;
    row = y / mapData.tileHeight;

    // clamp
    if (col < 0) col = 0;
    if (row < 0) row = 0;
    if (col >= mapData.width) col = mapData.width - 1;
    if (row >= mapData.height) row = mapData.height - 1;
}

bool Map::IsCollisionTileAt(int row, int col) const
{
    if (row < 0 || col < 0 || row >= mapData.height || col >= mapData.width) return false;

    for (const auto& layer : mapData.layers) {
        if (layer->name == "Collisions") {
            unsigned int gid = layer->Get(row, col);
            if (gid != 0) return true;
            // otherwise empty -> not collision
            return false;
        }
    }

    // If no Collisions layer present, assume not colliding
    return false;
}

int Map::GetTileWidth() const
{
    return mapData.tileWidth;
}

int Map::GetTileHeight() const
{
    return mapData.tileHeight;
}

int Map::GetWidth() const
{
    return mapData.width;
}

int Map::GetHeight() const
{
    return mapData.height;
}

