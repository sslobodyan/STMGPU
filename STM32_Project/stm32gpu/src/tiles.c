#include <string.h>
#include <stm32f10x.h>

#include <gfx.h>

#if USE_FSMC
 #include <fsmcdrv.h>
#else
 #include <spi.h>
#endif
#include <memHelper.h>

#include <raycast.h>

#include "tiles.h"
#include "nesPalette_ext.h"
#include "STMsGPU_Palette.h"

// -------------------------------------------------------- //

// RAM size is limited and share to all type of sprites!
uint8_t tileArr8x8[TILES_NUM_8x8][TILE_ARR_8X8_SIZE] = {0, 0};
uint8_t tileArr16x16[TILES_NUM_16x16][TILE_ARR_16X16_SIZE] = {0, 0};

// total: 640
uint16_t lastTile8x8[TILE_ARR_8X8_SIZE];       // 128
uint16_t lastTile16x16[TILE_ARR_16X16_SIZE];   // 512

// last tile id in tileArrYxY
uint8_t lastTileNum8x8 = 0xFF;
uint8_t lastTileNum16x16 = 0xFF;

#ifdef STM32F10X_HD
uint8_t tileArr32x32[TILES_NUM_32x32][TILE_ARR_32X32_SIZE] = {0, 0};
uint16_t lastTile32x32[TILE_ARR_32X32_SIZE];   // 2048
uint8_t lastTileNum32x32 = 0xFF;
#endif  /* STM32F10X_HD */


// on screen tile map background
// total: 1200 ( if 40x30 )
uint8_t mainBackGround[BACKGROUND_SIZE] = {0};

// total: 512
uint16_t currentPaletteArr[USER_PALETTE_SIZE]; // user can specify max 256 colors

uint8_t palChanged =0; // status flag; force to redraw tiles

// -------------------------------------------------------- //
/* as raycaster engine isolated, 
 * then need some accses to main global data
 * this is one of the ways
 */
void initRaycasterPointers(void)
{
  setRayCastPalette(currentPaletteArr);
  setLevelMap(mainBackGround);
  
#ifdef STM32F10X_HD
  setTileArrayPonter((uint8_t*)tileArr32x32, 3);
#endif
  setTileArrayPonter((uint8_t*)tileArr8x8, 1);
  setTileArrayPonter((uint8_t*)tileArr16x16, 2); // last one as default value
}

// load to RAM built-in palette
void loadDefaultPalette(void)
{
  // 80 colors and each 2 byte in size
  memcpy32(currentPaletteArr, nesPalette_ext, 160);
  
  // 234 colors and each 2 byte in size
  //memcpy32(currentPaletteArr, STMsGPU_234palette, STMSGPU_PALETTE*2);
}

void loadInternalTileSet(uint8_t tileSetSize, uint8_t tileSetW, const uint8_t *pTileSet)
{
  uint8_t tileYcnt, tileXcnt;    
  uint16_t tileNumOffsetX;       // offset for new scanline
  uint16_t offset;
  uint8_t *pTile = NULL;
  const uint8_t *pTileSetOffset = NULL;
  
  // offset for new scanline
  uint16_t tileNewLineOffsetY = tileSetW*TILE_8_BASE_SIZE-TILE_8_BASE_SIZE;
  
  for(uint8_t tileNum=0; tileNum <= tileSetSize; tileNum++) {
    
    pTile = tileArr8x8[tileNum];  // get RAM pointer where place tile data
    
    tileNumOffsetX = (tileNum % tileSetW)*TILE_8_BASE_SIZE;     // start position
    
    offset = ((tileNum)  / (tileSetW)) * (tileSetW * TILE_ARR_8X8_SIZE); 
    
    pTileSetOffset = &pTileSet[tileNumOffsetX + offset];  // set pointer to beginning of tile
    
    for(tileYcnt = 0; tileYcnt < TILE_8_BASE_SIZE; tileYcnt++) {  // Y
      for(tileXcnt =0; tileXcnt < TILE_8_BASE_SIZE; tileXcnt++) { // X
        *pTile++ = *pTileSetOffset++; // read single line in X pos
      }
      pTileSetOffset += tileNewLineOffsetY; // make new line
    }
  }
}

// -------------------------------------------------------- //

// return the pointer to given tile array number
uint8_t *getArrTilePointer8x8(uint8_t tileNum)
{
  return tileArr8x8[tileNum];
}

uint8_t *getArrTilePointer16x16(uint8_t tileNum)
{
  return tileArr16x16[tileNum];
}

#ifdef STM32F10X_HD
uint8_t *getArrTilePointer32x32(uint8_t tileNum)
{
  return tileArr32x32[tileNum];
}
#endif /* STM32F10X_HD */

uint8_t *getMapArrPointer(void)
{
  return mainBackGround;
}

void drawTile8x8(void *tileData)
{
  uint16_t *pTileData = (uint16_t *)tileData;
  
  int16_t posX = *pTileData++;
  int16_t posY = *pTileData++;
  uint8_t tileNum = *pTileData;
  
  // little trick, if tile same, just redraw it
  if(lastTileNum8x8 != tileNum) { // same tile number?
    
    uint8_t *pTileArr = &tileArr8x8[tileNum][0]; // get pointer by *pTile tile number
    lastTileNum8x8 = tileNum; // apply last tile number
    
    // convert colors from indexes in current palette to RGB565 color space
    for(uint16_t count =0; count < TILE_ARR_8X8_SIZE; count++) {
      lastTile8x8[count] = currentPaletteArr[*pTileArr++];
    }
  }
  
  // on oscilloscope this one reduce few uS
  setSqAddrWindow(posX, posY, TILE_8x8_WINDOW_SIZE);
  SEND_ARR16_FAST(lastTile8x8, TILE_ARR_8X8_SIZE);
}

void drawTile16x16(void *tileData)
{
  uint16_t *pTileData = (uint16_t *)tileData;
  
  int16_t posX = *pTileData++;
  int16_t posY = *pTileData++;
  uint8_t tileNum = *pTileData++;
  
  // little trick, if tile same, just redraw it
  if(lastTileNum16x16 != tileNum) {
    
    uint8_t *pTileArr = &tileArr16x16[tileNum][0];
    lastTileNum16x16 = tileNum;
    
    // convert colors from indexes in current palette to RGB565 color space
    for(uint16_t count =0; count < TILE_ARR_16X16_SIZE; count++) {
      lastTile16x16[count] = currentPaletteArr[*pTileArr++];
    }
  }
  
  // on oscilloscope this one reduce few uS
  setSqAddrWindow(posX, posY, TILE_16x16_WINDOW_SIZE);
  SEND_ARR16_FAST(lastTile16x16, TILE_ARR_16X16_SIZE);
}


#ifdef STM32F10X_HD
void drawTile32x32(void *tileData)
{
  uint16_t *pTileData = (uint16_t *)tileData;
  
  int16_t posX = *pTileData++;
  int16_t posY = *pTileData++;
  uint8_t tileNum = *pTileData++;
  
  // little trick, if tile same, just redraw it
  if(lastTileNum32x32 != tileNum) {
    
    uint8_t *pTileArr = &tileArr32x32[tileNum][0];
    lastTileNum32x32 = tileNum;
    
    // convert colors from indexes in current palette to RGB565 color space
    for(uint16_t count =0; count < TILE_ARR_32X32_SIZE; count++) {
      lastTile32x32[count] = currentPaletteArr[*pTileArr++];
    }
  }
  
  // on oscilloscope this one reduce few uS
  setSqAddrWindow(posX, posY, TILE_32x32_WINDOW_SIZE);
  SEND_ARR16_FAST(lastTile32x32, TILE_ARR_32X32_SIZE);
}
#endif /* STM32F10X_HD */

// -------------------------------------------------------- //

// Draw only 8x8 tiled background
void drawBackgroundMap(void)
{
  int16_t posX, posY;
  uint16_t tileMapCount =0;
  uint8_t tileX, tileY;
  uint8_t tileNum;
  uint8_t *pTileArr = NULL;
  
  for(tileY=0; tileY < BACKGROUND_SIZE_H; tileY++) {
    for(tileX=0; tileX < BACKGROUND_SIZE_W; tileX++) {
      
      posX = ( tileX * TILE_8_BASE_SIZE );
      posY = ( tileY * TILE_8_BASE_SIZE );
      tileNum = mainBackGround[tileMapCount];
      
      // little trick, if tile same, just redraw it
      if(lastTileNum8x8 != tileNum) { // same tile number?
        
        pTileArr = &tileArr8x8[tileNum][0]; // get pointer by *pTile tile number
        lastTileNum8x8 = tileNum; // apply last tile number
        
        // convert colors from indexes in current palette to RGB565 color space
        for(uint16_t count =0; count < TILE_ARR_8X8_SIZE; count++) {
          lastTile8x8[count] = currentPaletteArr[*pTileArr++];
        }
      }
      
      // on oscilloscope this one reduce few uS
      setSqAddrWindow(posX, posY, TILE_8x8_WINDOW_SIZE);
      SEND_ARR16_FAST(lastTile8x8, TILE_ARR_8X8_SIZE);
      
      ++tileMapCount;
    }
  }
}
