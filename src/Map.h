#pragma once

#include "Module.h"
#include <list>
#include <vector>

struct Properties
{
    struct Property
    {
        std::string name;
        bool value; 
    };

    std::list<Property*> propertyList;

    ~Properties()
    {
        for (const auto& property : propertyList)
        {
            delete property;
        }

        propertyList.clear();
    }

    Property* GetProperty(const char* name)
    {
        for (const auto& property : propertyList) {
            if (property->name == name) {
                return property;
            }
        }

        return nullptr;
    }

};

struct MapLayer
{
    int id;
    std::string name;
    int width;
    int height;
    std::vector<int> tiles;
    Properties properties;

    unsigned int Get(int i, int j) const
    {
        return tiles[(i * width) + j];
    }
};


struct TileSet
{
    int firstGid;
    std::string name;
    int tileWidth;
    int tileHeight;
    int spacing;
    int margin;
    int tileCount;
    int columns;
    SDL_Texture* texture;

    
    SDL_Rect GetRect(unsigned int gid) {
        SDL_Rect rect = { 0 };

        int relativeIndex = gid - firstGid;
        rect.w = tileWidth;
        rect.h = tileHeight;
        rect.x = margin + (tileWidth + spacing) * (relativeIndex % columns);
        rect.y = margin + (tileHeight + spacing) * (relativeIndex / columns);

        return rect;
    }

};


struct MapData
{
	int width;
	int height;
	int tileWidth;
	int tileHeight;
    std::list<TileSet*> tilesets;

    std::list<MapLayer*> layers;
};

class Map : public Module
{
public:

    Map();

    virtual ~Map();

    bool Awake();

    bool Start();

    bool Update(float dt);

    bool CleanUp();

    bool Load(std::string path, std::string mapFileName);

    Vector2D MapToWorld(int i, int j) const;
    TileSet* GetTilesetFromTileId(int gid) const;

  
    bool LoadProperties(pugi::xml_node& node, Properties& properties);

	Vector2D GetMapSizeInPixels();

    void WorldToMap(int x, int y, int& row, int& col) const;

    bool IsCollisionTileAt(int row, int col) const;

    int GetTileWidth() const;
    int GetTileHeight() const;
    int GetWidth() const;
    int GetHeight() const;

public: 
    std::string mapFileName;
    std::string mapPath;

private:

    bool mapLoaded;
 
    MapData mapData;

    SDL_Texture* helpTexture;        
    bool showHelpTexture;           
    SDL_Rect helpTextureRect;       
};