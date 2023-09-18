#include <cstdlib>
#include <cassert>


//
// file I/O


u8 *LoadFileMMAP(char *filepath, u64 *size_bytes);
StrLst GetFilesInFolderPaths(MArena *a, char *rootpath);


u32 LoadFileFSeek(char* filepath, u8* dest) {
    assert(dest != NULL && "data destination must be valid");
    u32 len = 0;

    FILE * f = fopen(filepath, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        len = ftell(f);
        fseek(f, 0, SEEK_SET);
        fread(dest, 1, len, f);
        fclose(f);
    }
    else {
        printf("LoadFileFSeek: Could not open file: %s\n", filepath);
    }

    return len;
}
bool SaveFile(char *filepath, u8 *data, u32 len) {
    FILE *f = fopen(filepath, "w");

    if (f == NULL) {
        printf("SaveFile: Could not open file %s\n", filepath);
        return false;
    }

    fwrite(data, 1, len, f);
    fclose(f);

    return true;
}


//
// random


#ifndef ULONG_MAX
#  define ULONG_MAX ((unsigned long)0xffffffffffffffffUL)
#endif

void Kiss_SRandom(unsigned long state[7], unsigned long seed) {
    if (seed == 0) seed = 1;
    state[0] = seed | 1; // x
    state[1] = seed | 2; // y
    state[2] = seed | 4; // z
    state[3] = seed | 8; // w
    state[4] = 0;        // carry
}
unsigned long Kiss_Random(unsigned long state[7]) {
    state[0] = state[0] * 69069 + 1;
    state[1] ^= state[1] << 13;
    state[1] ^= state[1] >> 17;
    state[1] ^= state[1] << 5;
    state[5] = (state[2] >> 2) + (state[3] >> 3) + (state[4] >> 2);
    state[6] = state[3] + state[3] + state[2] + state[4];
    state[2] = state[3];
    state[3] = state[6];
    state[4] = state[5] >> 30;
    return state[0] + state[1] + state[3];
}
unsigned long _hash(unsigned long x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x);
    return x;
}
unsigned long g_state[7];
bool g_didinit = false;
#define Random() Kiss_Random(g_state)
u32 RandInit(u32 seed = 0) {
    if (g_didinit == true)
        return 0;

    if (seed == 0) {
        seed = _hash(ReadCPUTimer());
    }
    Kiss_SRandom(g_state, seed);
    Kiss_Random(g_state); // flush the first one

    g_didinit = true;
    return seed;
}

double Rand01() {
    double randnum;
    randnum = (double) Random();
    randnum /= (double) ULONG_MAX + 1;
    return randnum;
}
double RandPM1() {
    double randnum;
    randnum = (double) Random();
    randnum /= ((double) ULONG_MAX + 1) / 2;
    randnum -= 1;
    return randnum;
}
int RandMinMaxI(int min, int max) {
    assert(max > min);
    return Random() % (max - min + 1) + min;
}
int RandDice(u32 max) {
    assert(max > 0);
    return Random() % max + 1;
}


void WriteRandomHexStr(char* dest, int nhexchars, bool put_newline_and_nullchar = true) {
    RandInit();

    for (int i = 0; i < nhexchars ; i++) {
        switch (RandMinMaxI(0, 15)) {
            case 0: { *dest = '0'; break; }
            case 1: { *dest = '1'; break; }
            case 2: { *dest = '2'; break; }
            case 3: { *dest = '3'; break; }
            case 4: { *dest = '4'; break; }
            case 5: { *dest = '5'; break; }
            case 6: { *dest = '6'; break; }
            case 7: { *dest = '7'; break; }
            case 8: { *dest = '8'; break; }
            case 9: { *dest = '9'; break; }
            case 10: { *dest = 'a'; break; }
            case 11: { *dest = 'b'; break; }
            case 12: { *dest = 'c'; break; }
            case 13: { *dest = 'd'; break; }
            case 14: { *dest = 'e'; break; }
            case 15: { *dest = 'f'; break; }
            default: { assert(1==0); break; }
        };
        dest++;
    }

    if (put_newline_and_nullchar) {
        *dest = '\n';
        dest++;
        *dest = '\0';
    }
}
