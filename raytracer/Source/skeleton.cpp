#include <iostream>
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include "SDLauxiliary.h"
#include "TestModelH.h"
#include <stdint.h>
#include <limits.h>
#include <omp.h>
using namespace std;
using glm::vec3;
using glm::mat3;
using glm::vec4;
using glm::mat4;


#define SCREEN_WIDTH 350 //100//320
#define SCREEN_HEIGHT 350 //100 //256
#define FULLSCREEN_MODE false
#define PI 3.1415926

struct Intersection
{
	vec4 position;
	float distance;
	int triangleIndex;
};


/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update();
void Draw(screen* screen);
bool ClosestIntersection(vec4 start, vec4 dir, const vector<Triangle>& triangles, Intersection* closestinter);
vec3 DirectLight(const Intersection *i);
vec3 IndirectLight(const Intersection *closestinter);


vector<Triangle> triangles;
float focalLength = 230;
vec4 cameraPos(0, 0, -2.5, 1.0);
vec4 lightPos(0, -0.5, -0.7, 1.0);
vec3 lightColor = 14.f * vec3(1,1,1);


int main( int argc, char* argv[] )
{
  
  screen *screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE );
  LoadTestModel(triangles);


  while( NoQuitMessageSDL() )
    {
      Update();
      Draw(screen);
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


  vec3 color(0, 0, 0);
  #pragma omp parallel 
  {

  for(int i=0; i<screen->width; i++){
    for(int j=0; j<screen->height; j++){
      bool result;
      Intersection closestinter;
      vec4 dir;  

      //compute the corresponding ray direction
      dir = vec4(i - SCREEN_WIDTH/2, j - SCREEN_HEIGHT/2, focalLength, 1);

	    //initialize the distance to the max
	    closestinter.distance = std::numeric_limits<float>::max();
      
      //compute the closest ray & find the color
      result = ClosestIntersection(cameraPos, dir, triangles, &closestinter);

      if(result == true){
    	  //color = triangles[closestinter.triangleIndex].color;
        color = DirectLight(&closestinter) + IndirectLight(&closestinter);
      } else {
      	color = vec3(0.0, 0.0, 0.0);
      }

      PutPixelSDL(screen, i, j, color);
    }
  }
  
  }
  
}


bool ClosestIntersection(vec4 start, vec4 dir, const vector<Triangle>& triangles, Intersection* closestinter){
	bool result = false;

	for( size_t i=0; i<triangles.size(); ++i )
	{
		vec4 v0 = triangles[i].v0;
		vec4 v1 = triangles[i].v1;
		vec4 v2 = triangles[i].v2;

		vec3 e1 = vec3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
		vec3 e2 = vec3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
		vec3 b = vec3(start.x - v0.x, start.y - v0.y, start.z - v0.z);
		vec3 d = vec3(dir.x, dir.y, dir.z);

		mat3 A(-d, e1, e2);	
		vec3 temp = glm::inverse(A) * b;

		if(temp.x >= 0 && temp.y >= 0 && temp.z >= 0 && temp.y + temp.z <= 1){
			result = true;
			if (temp.x <= closestinter->distance){
				closestinter->position.x = start.x + temp.x*dir.x;
				closestinter->position.y = start.y + temp.x*dir.y;
				closestinter->position.z = start.z + temp.x*dir.z;
				closestinter->position.w = 1.0;	
				closestinter->triangleIndex = i;
				closestinter->distance = temp.x; //t
			}
		}
		
	}
	return result;
}

vec3 DirectLight(const Intersection *i){
  vec4 dir;  
  bool result;
  Intersection closest;
  vec3 normal = vec3(triangles[i->triangleIndex].normal.x, triangles[i->triangleIndex].normal.y, triangles[i->triangleIndex].normal.z);
  vec4 position = i->position;


  //from light to surface
  dir = vec4(position.x - lightPos.x, position.y - lightPos.y, position.z - lightPos.z, 1);
  //reduce dir to length = 1
  //dir = dir / sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

  //initialize the distance to the max
  closest.distance = std::numeric_limits<float>::max();
  closest.triangleIndex = -1;
      
  //compute the closest ray & find if the closest
  result = ClosestIntersection(lightPos, dir, triangles, &closest);
 
  float i_l, closest_l;
  i_l = i->position.x * i->position.x + i->position.y * i->position.y + i->position.z * i->position.z;
  closest_l = closest.position.x * closest.position.x + closest.position.y * closest.position.y + closest.position.z * closest.position.z;

   
  //shadow
  if (result != true || i_l - closest_l > 0.01 || i_l - closest_l < -0.01){
    return vec3(0, 0, 0);
  }


  //the closest to the light, should be colored

  vec3 D;
  float k;
  vec3 r = vec3(lightPos.x - position.x, lightPos.y - position.y, lightPos.z - position.z);

  float r_2 = r.x * r.x + r.y * r.y + r.z * r.z;

  k = normal.x * r.x + normal.y * r.y + normal.z * r.z;
  k = (k > 0)? k : 0;
  k = k / (4 * PI * r_2);

  D = k * lightColor;
  D = D * triangles[i->triangleIndex].color;

  return D;
   
}


vec3 IndirectLight(const Intersection *closestinter){
  vec3 indirect = 0.5f * vec3(1, 1, 1); 
  indirect = indirect * triangles[closestinter->triangleIndex].color;
  return indirect;
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
  
  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  if(keystate[SDL_SCANCODE_UP]){
  	cameraPos = vec4(cameraPos.x, cameraPos.y , cameraPos.z + 0.5, cameraPos.w);
  }
  if(keystate[SDL_SCANCODE_DOWN]){
    cameraPos = vec4(cameraPos.x, cameraPos.y , cameraPos.z - 0.5, cameraPos.w);
  }
  if(keystate[SDL_SCANCODE_LEFT]){
    cameraPos = vec4(cameraPos.x - 0.5, cameraPos.y , cameraPos.z, cameraPos.w);
  }
  if(keystate[SDL_SCANCODE_RIGHT]){
    cameraPos = vec4(cameraPos.x + 0.5, cameraPos.y , cameraPos.z, cameraPos.w); 
  }
  if(keystate[SDL_SCANCODE_W]){
    lightPos -= vec4(0, 0, 0.5, 0);
  }
  if(keystate[SDL_SCANCODE_S]){
    lightPos += vec4(0, 0, 0.5, 0);
  }
}
