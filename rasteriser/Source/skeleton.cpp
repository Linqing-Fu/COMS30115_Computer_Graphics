#include <iostream>
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include "SDLauxiliary.h"
#include "TestModelH.h"
#include <stdint.h>
#include <math.h>
using namespace std;
using glm::vec3;
using glm::mat3;
using glm::vec4;
using glm::vec2;
using glm::mat4;
using glm::ivec2;

#define SCREEN_WIDTH 555
#define SCREEN_HEIGHT 555
#define FULLSCREEN_MODE false
#define PI 3.14159265


/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */


vector<Triangle> triangles;
float depthBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];
vec4 cameraPos(0, 0, -2.5, 1);
vec4 lightPos(0, -0.5, -0.7, 1);
vec3 lightPower = 14.f * vec3(1, 1, 1);
vec3 indirectLightPowerPerArea = 0.5f * vec3(1, 1, 1);
float focalLength = 230;
float yaw = 0;
mat3 R(vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
SDL_Surface *textureSurface;
Triangle *currentTriangle;

struct Pixel{
  int x;
  int y;
  float zinv;
  vec4 pos3d;
  vec2 texturePosition;
};

struct Vertex{
  vec4 position;
  // vec4 normal;
  // vec3 reflectance;
};


void Update();
void Draw(screen* screen);
void VertexShader(const Vertex& v, ivec2 &p, int vertexnum);
void DrawPolygonEdges(const vector<vec4> &vertices, screen* screen);
void DrawLineSDL(screen* screen, ivec2 a, ivec2 b, vec3 color);
void TransformationMatrix();

//void Interpolate(ivec2 a, ivec2 b, vector<ivec2> &result);
//void DrawPolygon(const vector<vec4> &vertices, screen* screen, glm::vec3 currentColor);
//void ComputePolygonRows(const vector<ivec2> &vertexPixels, vector<ivec2> &leftPixels, vector<ivec2> &rightPixels);
//void DrawPolygonRows(const vector<ivec2> &leftPixels, const vector<ivec2> &rightPixels, screen* screen, glm::vec3 currentColor);


void Interpolate(Pixel a, Pixel b, vector<Pixel> &result);
void DrawPolygon(const vector<Vertex> &vertices, screen* screen, vec4 currentNormal, vec3 currentReflectance);
void ComputePolygonRows(const vector<Pixel> &vertexPixels, vector<Pixel> &leftPixels, vector<Pixel> &rightPixels);
void DrawPolygonRows(const vector<Pixel> &leftPixels, const vector<Pixel> &rightPixels, screen* screen, vec4 currentNormal, vec3 currentReflectance);
void PixelShader(const Pixel& p, screen* screen, vec4 currentNormal, vec3 currentReflectance);

int main( int argc, char* argv[] )
{
  
  screen *screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE );
  LoadTestModel(triangles);
  
  const char file[] = "./space.bmp";
  textureSurface = SDL_LoadBMP(file);
  if(textureSurface == NULL){
    cout << "Fail to load texture" << endl;
  } else {
    cout << textureSurface->h << endl;
  }

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

  //initial depth buffer
  for(int y = 0; y < SCREEN_HEIGHT; ++y){
    for(int x = 0; x < SCREEN_WIDTH; ++x){
      depthBuffer[y][x] = 0;
    }
  }
  

  for(uint32_t i = 0; i < triangles.size(); ++i){
    vector<Vertex> vertices(3);
    vertices[0].position = triangles[i].v0;
    vertices[1].position = triangles[i].v1;
    vertices[2].position = triangles[i].v2;
  
    vec4 currentNormal = triangles[i].normal;
    vec3 currentReflectance = triangles[i].color;  
    currentTriangle = &triangles[i];
    
    DrawPolygon(vertices, screen, currentNormal, currentReflectance);
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

 const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  if(keystate[SDL_SCANCODE_LEFT]){
    yaw -= 0.0001;
    TransformationMatrix();
    vec4 right(R[0][0], R[0][1], R[0][2], 0);
    cameraPos = vec4(cameraPos - right);//vec4(cameraPos.x - 0.5, cameraPos.y , cameraPos.z, cameraPos.w);
  }
  if(keystate[SDL_SCANCODE_RIGHT]){
    yaw += 0.0001;
    TransformationMatrix();
    vec4 right(R[0][0], R[0][1], R[0][2], 0);
    cameraPos = vec4(cameraPos + right);//(cameraPos.x + 0.5, cameraPos.y , cameraPos.z, cameraPos.w); 
  }
  if(keystate[SDL_SCANCODE_W]){
    lightPos -= vec4(0, 0, 0.5, 0);
  }
  if(keystate[SDL_SCANCODE_S]){
    lightPos += vec4(0, 0, 0.5, 0);
  }
  if(keystate[SDL_SCANCODE_A]){
    lightPos -= vec4(0.5, 0, 0, 0);
  }
  if(keystate[SDL_SCANCODE_D]){
    lightPos += vec4(0.5, 0, 0, 0);
  }

}




void VertexShader(const Vertex& v, Pixel& p, int vertexnum){
  vec4 v_change;
  //v_change = vec4(v.x - cameraPos.x, v.y - cameraPos.y, v.z - cameraPos.z, v.w - cameraPos.w);
  v_change = vec4(v.position - cameraPos);

  
  //mul R
  vec4 e1(R[0][0], R[0][1], R[0][2], 0);
  vec4 e2(R[1][0], R[1][1], R[1][2], 0);
  vec4 e3(R[2][0], R[2][1], R[2][2], 0);
  vec4 e4(0, 0, 0, 1);
  mat4 R_homo(e1, e2, e3, e4);
  v_change = R_homo * v_change;


  p.x = (SCREEN_HEIGHT * v_change.x / (2 * v_change.z) + SCREEN_WIDTH / 2);
  p.y = (SCREEN_HEIGHT * v_change.y / (2 * v_change.z) + SCREEN_HEIGHT / 2);
  p.zinv = 1 / v_change.z;
  p.pos3d = v_change;

  //texture
  if(vertexnum == 0){
    p.texturePosition = currentTriangle->v0_tp;
  } else if(vertexnum == 1){
    p.texturePosition = currentTriangle->v1_tp;
  } else if(vertexnum == 2){
    p.texturePosition = currentTriangle->v2_tp;
  }

}


void Interpolate(Pixel a, Pixel b, vector<Pixel> &result){
  int N = result.size();
  vec2 step = vec2(b.x - a.x, b.y - a.y) / float(max(N-1, 1));
  float depth = (b.zinv - a.zinv) / float(max(N-1, 1));

  vec4 pos = vec4(b.pos3d * b.zinv - a.pos3d * a.zinv) / float(max(N - 1, 1));
  vec2 texture_step = vec2(b.texturePosition - a.texturePosition) / float(max(N - 1, 1));
  Pixel current = a;

  vec4 q = current.pos3d * current.zinv;

  for(int i = 0; i < N; i++){
    result[i] = current;
    current.x += step.x;
    current.y += step.y;
    current.zinv += depth;
    q += pos;
    current.pos3d = q / current.zinv;
    current.texturePosition += texture_step;
    // printf("result.zinv %f\n", result[i].zinv);
  }
}


void TransformationMatrix(){
  vec3 e1(cos(yaw * PI / 180.0), 0, sin(yaw * PI / 180.0));
  vec3 e2(0, 1, 0);
  vec3 e3(-sin(yaw * PI / 180.0), 0, cos(yaw * PI / 180.0));
  R = mat3(e1, e2, e3);
}



void DrawPolygon(const vector<Vertex> &vertices, screen* screen, vec4 currentNormal, vec3 currentReflectance){
  int V = vertices.size();
  // int V = sizeof(vertices) / sizeof(Vertex[0]);
  // printf("%ld  %ld\n",sizeof(vertices), sizeof(Vertex[0]));

  vector<Pixel> vertexPixels(V);
  for(int i = 0; i < V; i++){
    VertexShader(vertices[i], vertexPixels[i], i);
  }

  vector<Pixel> leftPixels;
  vector<Pixel> rightPixels;
  ComputePolygonRows(vertexPixels, leftPixels, rightPixels);
  DrawPolygonRows(leftPixels, rightPixels, screen, currentNormal, currentReflectance);
  // leftPixels.clear();
  // rightPixels.clear();
}


void ComputePolygonRows(const vector<Pixel> &vertexPixels, vector<Pixel> &leftPixels, vector<Pixel> &rightPixels){
  int V = vertexPixels.size();

  //find the max and min y
  int max_y = std::max(vertexPixels[0].y, vertexPixels[1].y);
  max_y = std::max(max_y, vertexPixels[2].y);
  int min_y = std::min(vertexPixels[0].y, vertexPixels[1].y);
  min_y = std::min(min_y, vertexPixels[2].y);
  int rows = max_y - min_y + 1;

  //reserve size
  leftPixels.resize(rows);
  rightPixels.resize(rows);

  //initialize x in left to largest, in right to smallest
  for(int i = 0; i < rows; i++){
    leftPixels[i].x = numeric_limits<int>::max();
    rightPixels[i].x = -numeric_limits<int>::max();

  }

  //loop through all edge
  for(int i = 0; i < V; i++){
    Pixel a = vertexPixels[i];
    Pixel b = vertexPixels[(i + 1) % V];
    float step = 1.f / (glm::length(vec2(a.x - b.x, a.y - b.y)) + 1);

    for(float t = 0; t < 1; t += step){
      Pixel pixel;

      pixel.x = static_cast<int>(a.x + t * (b.x - a.x));
      pixel.y = static_cast<int>(a.y + t * (b.y - a.y));
      pixel.zinv = a.zinv + t * (b.zinv - a.zinv);
      pixel.pos3d = ((a.pos3d * a.zinv) + t * (b.pos3d * b.zinv - a.pos3d * a.zinv)) / pixel.zinv;
      pixel.texturePosition = a.texturePosition + t * (b.texturePosition - a.texturePosition);

      if(pixel.x < 0){
        pixel.x = 0;
      } else if (pixel.x > SCREEN_WIDTH) {
        pixel.x = SCREEN_WIDTH;
      }

      if(pixel.y < 0){
        pixel.y = 0;
      } else if (pixel.y > SCREEN_HEIGHT) {
        pixel.y = SCREEN_HEIGHT;
      }

      //update left and right
      int y = pixel.y - min_y;
      if(pixel.x <= leftPixels[y].x){
        leftPixels[y] = pixel;
      }
      if(pixel.x >= rightPixels[y].x){
        rightPixels[y] = pixel;
      }
    }
  }


}



void DrawPolygonRows(const vector<Pixel> &leftPixels, const vector<Pixel> &rightPixels, screen* screen, vec4 currentNormal, vec3 currentReflectance){
  int size = leftPixels.size();
  vector<Pixel> line;
  
  // for(int i = 0; i < size; i ++){
  //   printf("left.y %d\n", leftPixels[i].y);
  // }
  for(int i = 0; i < size; i++){
    ivec2 delta = glm::abs(ivec2(leftPixels[i].x - rightPixels[i].x, leftPixels[i].y - rightPixels[i].y));
    //printf("leftPixel.x: %d  right.x: %d\n", leftPixels[i].x, rightPixels[i].x);
    // printf("i :%d leftPixel.y: %d  right.y: %d\n",i, leftPixels[i].y, rightPixels[i].y);
    
    int pixels = glm::max(delta.x, delta.y) + 1;

    /////////////error alloc
    line.clear();
    line.resize(pixels);

    Interpolate(leftPixels[i], rightPixels[i], line);

    for(int j = 0; j < pixels; j++){
      PixelShader(line[j], screen, currentNormal, currentReflectance);
    }
  }
}

void PixelShader(const Pixel& p, screen* screen, vec4 currentNormal, vec3 currentReflectance){
  int x = p.x;
  int y = p.y;
  vec3 position(p.pos3d.x, p.pos3d.y, p.pos3d.z);
  vec3 illumination;
  vec3 light;
  vec3 texturecolor;

  if(depthBuffer[y][x] < p.zinv){
    //computer the light

    vec3 D;
    float k;

    light = vec3(lightPos.x - cameraPos.x, lightPos.y - cameraPos.y, lightPos.z - cameraPos.z);
    light = R * light;

    vec3 r = vec3(light.x - position.x, light.y - position.y, light.z - position.z);
    float r_2 = r.x * r.x + r.y * r.y + r.z * r.z;
  
    k = currentNormal.x * r.x + currentNormal.y * r.y + currentNormal.z * r.z;
    k = (k > 0)?k : 0;
    k = k / (4 * PI * r_2);

    D = k * lightPower;
    illumination = (D + indirectLightPowerPerArea);

    depthBuffer[y][x] = p.zinv;

    //texture
    texturecolor = currentReflectance;
    if(currentTriangle->texture){
      texturecolor = GetPixelSDL(textureSurface, (int)(p.texturePosition.x), (int)(p.texturePosition.y));  
    }
    PutPixelSDL(screen, x, y, illumination * texturecolor);
  }
}

