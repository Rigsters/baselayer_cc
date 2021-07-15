#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "types.h"
#include "memory.h"
#include "random.h"
#include "stuff.h"
#include "SDL2/SDL.h"


void TestRandGen() {
  RandInit();

  std::cout << "rand01():" << std::endl;
  for (int i = 0; i<20; i++) {
    std::cout << Rand01() << " ";
  }
  std::cout << std::endl << std::endl;

  std::cout << "randpm1():" << std::endl;
  for (int i = 0; i<20; i++) {
    std::cout << RandPM1() << " ";
  }
  std::cout << std::endl << std::endl;

  std::cout << "rand0max(10):" << std::endl;
  for (int i = 0; i<20; i++) {
    std::cout << Rand0Max(10) << " ";
  }
  std::cout << std::endl << std::endl;

  std::cout << "randminmax(100, 200):" << std::endl;
  for (int i = 0; i<20; i++) {
    std::cout << RandMinMax(100, 200) << " ";
  }
  std::cout << std::endl << std::endl;

  std::cout << "randtriangle():" << std::endl;
  for (int i = 0; i<20; i++) {
    std::cout << RandTriangle() << " ";
  }
  std::cout << std::endl << std::endl;

  std::cout << "randnorm():" << std::endl;
  for (int i = 0; i<20; i++) {
    std::cout << RandNorm() << " ";
  }
  std::cout << std::endl << std::endl;

  std::cout << "gaussian_double():" << std::endl;
  for (int i = 0; i<20; i++) {
    std::cout << RandGaussianDouble() << " ";
  }
  std::cout << std::endl << std::endl;

  std::cout << "randminmax_i(10, 20):" << std::endl;
  for (int i = 0; i<20; i++) {
    std::cout << RandMinMaxI(10, 20) << " ";
  }
  std::cout << std::endl << std::endl;
}


struct Entity {
  char* name;
  char* body;
};

void TestGPAlloc() {
  GeneralPurposeAllocator alloc( 1024 * 1024 );

  const char* s01 = "en spændende test streng";
  const char* s02 = "entity test string body indhold...";

  Entity my_entity;
  my_entity.name = (char*) alloc.Alloc(strlen(s01));
  my_entity.body = (char*) alloc.Alloc(strlen(s02));

  ListPrintSizes(alloc.blocks);

  strcpy(my_entity.name, s01);
  strcpy(my_entity.body, s02);
}


void TestIterateAndDisplayGPAList(GeneralPurposeAllocator* alloc) {
  MemoryBlock* lst = alloc->blocks;
  int idx = 0;
  do {
    std::cout << idx << " size: " << lst->total_size;

    if (lst->used == true)
      std::cout << " used" << std::endl;
    else
      std::cout << " not used" << std::endl;

    idx++;
    lst = lst->next;
  } while (lst != alloc->blocks);
}

void TestGPAlloc2WriteChars() {
  GeneralPurposeAllocator alloc( 1024 * 1024 );
  RandInit();

  for (int i = 0; i < 10; i++) {
    std::cout << RandMinMaxI(0, 10) << std::endl;
  }

  exit(0);

  int num_lines = 150;
  char* lines[num_lines];
  int line_len;

  int accum_totlen = 0;

  // fill up string locations
  for (int i = 0; i < num_lines; i++) {
    line_len = RandMinMaxI(50, 250);
    char* dest = (char*) alloc.Alloc(line_len + 1);
    WriteRandomHexStr(dest, line_len);
    lines[i] = dest;

    std::cout << dest << std::endl;
  }

  std::cout << std::endl;

  // free some of the lines
  int num_tofree = 60;
  int idx;
  for (int i = 0; i < num_tofree; i++) {

    idx = RandMinMaxI(0, num_lines - 1);
    std::cout << idx << " load: " << alloc.load << std::endl;

    bool success = alloc.Free(lines[idx]);
  }
  std::cout << std::endl << "total blocks merged:" << alloc.blocks_merged << std::endl;

  TestIterateAndDisplayGPAList(&alloc);
}


struct PoolAllocTestStruct {
  int some_num;
  char some_word[35];
};

void TestPoolAlloc() {
  int batch_size = 10;
  PoolAllocator palloc(sizeof(PoolAllocTestStruct), batch_size);

  void* ptrs[batch_size];
  std::cout << "pool load: " << palloc.load << " element size: " << sizeof(PoolAllocTestStruct) << std::endl;

  // allocate items
  for (int i = 0; i < batch_size; i++) {
    ptrs[i] = palloc.Alloc();
    int relloc = (u8*) ptrs[i] - (u8*) palloc.root;
    std::cout << "pool load: " << palloc.load << " - relloc: " << relloc << std::endl;
  }

  // free four items at stride 2
  for (int i = 0; i < batch_size - 2; i += 2) {
    palloc.Free(ptrs[i]);
    std::cout << "pool load: " << palloc.load << std::endl;
  }

  // put them back in
  for (int i = 0; i < batch_size - 2; i += 2) {
    ptrs[i] = palloc.Alloc();
    std::cout << "pool load: " << palloc.load << std::endl;
  }

  // it is full now
  assert(palloc.Alloc() == NULL);

  palloc.Free(ptrs[4]);
  std::cout << "pool load: " << palloc.load << std::endl;
  palloc.Free(ptrs[5]);
  std::cout << "pool load: " << palloc.load << std::endl;

  ptrs[5] = palloc.Alloc();
  std::cout << "pool load: " << palloc.load << std::endl;

  ptrs[4] = palloc.Alloc();
  std::cout << "pool load: " << palloc.load << std::endl;

  assert(palloc.Alloc() == NULL);
}


#define SIXTEEN_K 16 * 1024
void TestStackAlloc() {
  StackAllocator arena(SIXTEEN_K);
  std::cout << "initial arena used: " << arena.used << std::endl;

  char* strings[1000];
  int sidx = 0;

  // allocate a bunch of strings
  char* dest;
  int len = 0;
  RandInit();
  while (arena.total_size - arena.used > len) {
    std::cout << "used: " << arena.used << " total: " << arena.total_size <<  std::endl;

    dest = (char*) arena.Alloc(len);
    WriteRandomHexStr(dest, len);
    strings[sidx] = dest;
    sidx++;

    len = RandMinMaxI(50, 350);
  }

  std::cout << std::endl << "now printing all of those allocated strings:" << std::endl;;

  // print those strings
  for (int i = 0; i < sidx; i++) {
    std::cout << strings[i] << std::endl;
  }

  std::cout << std::endl << "unraveling down to the five first strings..." << std::endl;;

  // unravel the stack from the top, almost all of the way down (Free will actually memset to zero)
  for (int i = sidx - 1; i >= 5; i--) {
    arena.Free(strings[i]);
  }

  std::cout << std::endl << "printing those five strings:" << std::endl;;

  // print those strings
  for (int i = 0; i < 5; i++) {
    std::cout << strings[i] << std::endl;
  }

  std::cout << std::endl << "done." << std::endl;;
}


void TestLoadFile() {
  char* filename = "memory.c";
  char* text = LoadFile(filename);

  std::cout << text << std::endl;
}

#include "frametimer.h"

#define RGB_DEEP_BLUE 20, 50, 150
struct Color {
  u8 r = 20;
  u8 g = 50;
  u8 b = 150;
};
Color g_bckgrnd_color;
Color g_rect_color { 0, 255, 0 };

int MaxI(int a, int b) {
  if (a >= b)
    return a;
  else
    return b;
}
int MinI(int a, int b) {
  if (a <= b)
    return a;
  else
    return b;
}

int DoInput(void)
{
	SDL_Event event;
	
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				exit(0);
				break;
				
			case SDL_KEYDOWN:
        if(event.key.keysym.scancode == SDL_SCANCODE_Q)
          return -1;
        if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
          g_bckgrnd_color.b = (u8) MaxI(g_bckgrnd_color.b - 10, 0);
          std::cout << (int) g_bckgrnd_color.b << std::endl;
        }
        if (event.key.keysym.scancode == SDL_SCANCODE_UP) {
          g_bckgrnd_color.b = (u8) MinI(g_bckgrnd_color.b + 10, 255);
          std::cout << (int) g_bckgrnd_color.b << std::endl;
        }
				break;
			case SDL_KEYUP:
				break;

			default:
				break;
		}
	}
  return 0;
}

void DoRender(SDL_Renderer* renderer) {
  SDL_SetRenderDrawColor(renderer, g_bckgrnd_color.r, g_bckgrnd_color.g, g_bckgrnd_color.b, 255);
	SDL_RenderClear(renderer);

  SDL_Rect rect;
  rect.x = 250;
  rect.y = 150;
  rect.w = 200;
  rect.h = 200;

  SDL_SetRenderDrawColor(renderer, g_rect_color.r, g_rect_color.g, g_rect_color.b, 255);
  //SDL_RenderDrawRect(renderer, &rect);
  SDL_RenderFillRect(renderer, &rect);

  SDL_RenderPresent(renderer);
}

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

void TestOpenWindow() {
  SDL_Init(SDL_INIT_VIDEO);

  // window
  SDL_Window* window = SDL_CreateWindow(
    "Mimic",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    WINDOW_WIDTH, WINDOW_HEIGHT,
    SDL_WINDOW_OPENGL
  );

  // renderer
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  FrameTimer frame_tm(1/60);
  bool terminated = false;
  while (terminated != true) {
    if (DoInput() == -1) terminated = true;

    // TODO: do logics

    // TODO: do rendering
    DoRender(renderer);

    frame_tm.wait_for_frame();
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}


int main (int argc, char **argv) {
  //TestRandGen();
  //TestLoadFile();

  //TestGPAlloc();
  TestGPAlloc2WriteChars();
  //TestPoolAlloc();
  //TestStackAlloc();

  //TestOpenWindow();
}
