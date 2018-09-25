#include <iostream>
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"
#include <stdint.h>

using namespace std;
using glm::vec3;
using glm::mat3;

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 256
#define FULLSCREEN_MODE false


/* ----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES                                                            */
int t;

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update();
void Draw(screen* screen);
void Interpolate(vec3 a, vec3 b, vector<vec3>& result);

vector<vec3> stars(1000);
int main( int argc, char* argv[] )
{
  
  screen *screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE );
  t = SDL_GetTicks(); /*Set start value for timer.*/
  for(int i = 0; i < 1000; i++){
    stars[i].x = -1 + 2*float(rand())/float(RAND_MAX);
    stars[i].y = -1 + 2*float(rand())/float(RAND_MAX);
    stars[i].z = float(rand())/float(RAND_MAX);  
  }


  
  while( NoQuitMessageSDL() )
    {
      Draw(screen);
      Update();
      SDL_Renderframe(screen);
    }

  SDL_SaveImage( screen, "screenshot.bmp" );

  KillSDL(screen);
  return 0;
}

/*Place your drawing here*/
void Draw(screen* screen)
{
  /* Clear buffer */
  memset(screen->buffer, 0, screen->height*screen->width*sizeof(uint32_t));
  
  vec3 colour;
  for(size_t s = 0; s < stars.size(); s++){
    colour = vec3(0.2f/(stars[s].z * stars[s].z),0.2f/(stars[s].z * stars[s].z),0.2f/(stars[s].z * stars[s].z));
    
    uint32_t u = (SCREEN_HEIGHT * stars[s].x / (2 * stars[s].z) + SCREEN_WIDTH/2);
    uint32_t v = (SCREEN_HEIGHT * stars[s].y / (2 * stars[s].z) + SCREEN_WIDTH/2);
    PutPixelSDL(screen, u, v, colour);
  }
  
  /*
  //rainbow
  vec3 topLeft(1, 0, 0);
  vec3 topRight(0, 0, 1);
  vec3 bottomLeft(0, 1, 0);
  vec3 bottomRight(1, 1, 0);
  vector<vec3> leftSide(SCREEN_HEIGHT);
  vector<vec3> rightSide(SCREEN_HEIGHT);
  vector<vec3> colour(SCREEN_WIDTH);
  Interpolate(topLeft, bottomLeft, leftSide);
  Interpolate(topRight, bottomRight, rightSide);

  for(int i = 0; i < SCREEN_HEIGHT; i++){
    Interpolate(leftSide[i], rightSide[i], colour);
    for(int j = 0; j < SCREEN_WIDTH; j++){
      PutPixelSDL(screen, j, i, colour[j]);
    }
  }
  
*/
}
/*
  for(int i=0; i<1000; i++)
    {
      //uint32_t x = 2*i%screen->width;//rand() % screen->width;
      //uint32_t y = 35;//rand() % screen->height;      
  PutPixelSDL(screen, x, y, colour);
    }


  for(int i=0; i<screen->width; i++){
    for(int j=0; j<screen->height; j++){
      vector<float> result(10);
      uint32_t x = i;
      uint32_t y = j;     
      PutPixelSDL(screen, x, y, colour);
    }
  }
  */

void Interpolate(vec3 a, vec3 b, vector<vec3>& result){
  uint32_t size = result.size();
  if(size == 1){
    result[0].x = (a.x + b.x)/2;
    result[0].y = (a.y + b.y)/2;
    result[0].z = (a.z + b.z)/2;
  } else {
    for(uint32_t i = 0; i < size; i++){
      result[i].x = a.x + (b.x - a.x) * i/(size - 1);
      result[i].y = a.y + (b.y - a.y) * i/(size - 1);
      result[i].z = a.z + (b.z - a.z) * i/(size - 1);
    }
  }
}

/*Place updates of parameters here*/
void Update()
{
  static int t = SDL_GetTicks();
  /* Compute frame time */
  int t2 = SDL_GetTicks();
  float dt = float(t2-t);
  t = t2;
  /*Good idea to remove this*/
  std::cout << "Render time: " << dt << " ms." << std::endl;
  /* Update variables*/
  for(uint32_t s = 0; s < stars.size(); s++){
    
    stars[s].z = stars[s].z - dt*0.001;
    if(stars[s].z <= 0){
      stars[s].z += 1;
    } else if (stars[s].z > 1){
      stars[s].z -= 1;
    }
  }
}
