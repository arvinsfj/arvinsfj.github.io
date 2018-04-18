#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cJSON.h"

/* Sectors: Floor and ceiling height; list of edge vertices and neighbors */
static struct sector
{
    float floor, ceil;
    struct xy { float x,y; } *vertex; // Each vertex has an x and y coordinate
    signed char *neighbors;           // Each edge may have a corresponding neighboring sector
    unsigned npoints;                 // How many vertexes there are
} *sectors = NULL;
static unsigned NumSectors = 0;

/* Player: location */
static struct player
{
    struct xyz { float x,y,z; } where,      // Current position
    velocity;   // Current motion vector
    float angle, anglesin, anglecos, yaw;   // Looking towards (and sin() and cos() thereof)
    unsigned sector;                        // Which sector the player is currently in
} player;

static void LoadData()
{
    FILE* fp = fopen("map-clear.txt", "rt");
    if(!fp) { perror("map-clear.txt"); exit(1); }
    char Buf[256], word[256], *ptr;
    struct xy* vert = NULL, v;
    int n, m, NumVertices = 0;
    while(fgets(Buf, sizeof Buf, fp))
    switch(sscanf(ptr = Buf, "%32s%n", word, &n) == 1 ? word[0] : '\0')
    {
            case 'v': // vertex
            for(sscanf(ptr += n, "%f%n", &v.y, &n); sscanf(ptr += n, "%f%n", &v.x, &n) == 1; )
        { vert = realloc(vert, ++NumVertices * sizeof(*vert)); vert[NumVertices-1] = v; }
            break;
            case 's': // sector
            sectors = realloc(sectors, ++NumSectors * sizeof(*sectors));
            struct sector* sect = &sectors[NumSectors-1];
            int* num = NULL;
            sscanf(ptr += n, "%f%f%n", &sect->floor,&sect->ceil, &n);
            for(m=0; sscanf(ptr += n, "%32s%n", word, &n) == 1 && word[0] != '#'; )
        { num = realloc(num, ++m * sizeof(*num)); num[m-1] = word[0]=='x' ? -1 : atoi(word); }
            sect->npoints   = m /= 2;
            sect->neighbors = malloc( (m  ) * sizeof(*sect->neighbors) );
            sect->vertex    = malloc( (m+1) * sizeof(*sect->vertex)    );
            for(n=0; n<m; ++n) sect->neighbors[n] = num[m + n];
            for(n=0; n<m; ++n) sect->vertex[n+1]  = vert[num[n]]; // TODO: Range checking
            sect->vertex[0] = sect->vertex[m]; // Ensure the vertexes form a loop
            free(num);
            break;
            case 'p':; // player
            float angle;
            sscanf(ptr += n, "%f %f %f %d", &v.x, &v.y, &angle,&n);
            player = (struct player) { {v.x, v.y, 0}, {0,0,0}, angle,0,0,0, n }; // TODO: Range checking
            player.where.z = sectors[player.sector].floor + 10;
    }
    fclose(fp);
    free(vert);
}

static void UnloadData()
{
    for(unsigned a=0; a<NumSectors; ++a) free(sectors[a].vertex);
    for(unsigned a=0; a<NumSectors; ++a) free(sectors[a].neighbors);
    free(sectors);
    sectors    = NULL;
    NumSectors = 0;
}

int main(int argc, char** argv)
{
    LoadData();
    //
    char *out;
    cJSON* root =  cJSON_CreateArray();
    for (int i = 0; i < NumSectors; ++i) {
        struct sector* sect = &sectors[i];
        cJSON* item_sect =  cJSON_CreateObject();
        cJSON* item_arr =  cJSON_CreateArray();
        cJSON_AddItemToObject(item_sect, "sector", item_arr);
        cJSON_AddItemToArray(root, item_sect);
        for (int j = 0; j < sect->npoints; ++j) {
            struct xy* point = &(sect->vertex[j]);
            cJSON* item_point =  cJSON_CreateObject();
            cJSON_AddItemToObject(item_point, "x", cJSON_CreateNumber(point->x));
            cJSON_AddItemToObject(item_point, "y", cJSON_CreateNumber(point->y));
            cJSON_AddItemToArray(item_arr, item_point);
        }
    }
    out=cJSON_PrintUnformatted(root);    cJSON_Delete(root);    printf("%s\n",out);    free(out);
    //
    UnloadData();
    return 0;
}
