#pragma once
#include <BleComboKeyboard.h>

// Codici HID per lettere e numeri base
#define KEY_A 0x04
#define KEY_B 0x05
#define KEY_C 0x06
#define KEY_D 0x07
#define KEY_E 0x08
#define KEY_F 0x09
#define KEY_G 0x0A
#define KEY_H 0x0B
#define KEY_I 0x0C
#define KEY_J 0x0D
#define KEY_K 0x0E
#define KEY_L 0x0F
#define KEY_M 0x10
#define KEY_N 0x11
#define KEY_O 0x12
#define KEY_P 0x13
#define KEY_Q 0x14
#define KEY_R 0x15
#define KEY_S 0x16
#define KEY_T 0x17
#define KEY_U 0x18
#define KEY_V 0x19
#define KEY_W 0x1A
#define KEY_X 0x1B
#define KEY_Y 0x1C
#define KEY_Z 0x1D

#define KEY_1 0x1E
#define KEY_2 0x1F
#define KEY_3 0x20
#define KEY_4 0x21
#define KEY_5 0x22
#define KEY_6 0x23
#define KEY_7 0x24
#define KEY_8 0x25
#define KEY_9 0x26
#define KEY_0 0x27

#define KEY_ENTER 0x28
#define KEY_ESC 0x29
#define KEY_BACKSPACE 0x2A
#define KEY_TAB 0x2B
#define KEY_SPACE 0x2C
#define KEY_MINUS 0x2D
#define KEY_EQUAL 0x2E
#define KEY_LEFT_BRACE 0x2F
#define KEY_RIGHT_BRACE 0x30
#define KEY_BACKSLASH 0x31
#define KEY_SEMICOLON 0x33
#define KEY_QUOTE 0x34
#define KEY_TILDE 0x35
#define KEY_COMMA 0x36
#define KEY_PERIOD 0x37
#define KEY_SLASH 0x38
#define KEY_è 0x35
#define KEY_ù 0x36 
#define KEY_PLUS 0x37 

// Questa tabella converte i caratteri ASCII in tasti fisici US + SHIFT/ALT_GR necessari per ottenere il corretto simbolo italiano.
typedef struct {
  char input;
  uint8_t keycode;
  bool shift;
  bool altgr;
} KeyMapEntry;

static const KeyMapEntry IT_KEYMAP[] = {
  {'a', KEY_A, false, false}, {'A', KEY_A, true, false},
  {'b', KEY_B, false, false}, {'B', KEY_B, true, false},
  {'c', KEY_C, false, false}, {'C', KEY_C, true, false},
  {'d', KEY_D, false, false}, {'D', KEY_D, true, false},
  {'e', KEY_E, false, false}, {'E', KEY_E, true, false},
  {'f', KEY_F, false, false}, {'F', KEY_F, true, false},
  {'g', KEY_G, false, false}, {'G', KEY_G, true, false},
  {'h', KEY_H, false, false}, {'H', KEY_H, true, false},
  {'i', KEY_I, false, false}, {'I', KEY_I, true, false},
  {'l', KEY_L, false, false}, {'L', KEY_L, true, false},
  {'m', KEY_M, false, false}, {'M', KEY_M, true, false},
  {'n', KEY_N, false, false}, {'N', KEY_N, true, false},
  {'o', KEY_O, false, false}, {'O', KEY_O, true, false},
  {'p', KEY_P, false, false}, {'P', KEY_P, true, false},
  {'q', KEY_Q, false, false}, {'Q', KEY_Q, true, false},
  {'r', KEY_R, false, false}, {'R', KEY_R, true, false},
  {'s', KEY_S, false, false}, {'S', KEY_S, true, false},
  {'t', KEY_T, false, false}, {'T', KEY_T, true, false},
  {'u', KEY_U, false, false}, {'U', KEY_U, true, false},
  {'v', KEY_V, false, false}, {'V', KEY_V, true, false},
  {'z', KEY_Z, false, false}, {'Z', KEY_Z, true, false},
  {'x', KEY_X, false, false}, {'X', KEY_X, true, false},
  {'y', KEY_Y, false, false}, {'Y', KEY_Y, true, false},
  {' ', KEY_SPACE, false, false},
  {'.', KEY_PERIOD, false, false},
  {',', KEY_COMMA, false, false},
  {';', KEY_SEMICOLON, false, false},
  {':', KEY_PERIOD, true, false},       // SHIFT + .
  {'!', KEY_1, true, false},
  {'"', KEY_2, true, false},
  {'£', KEY_3, true, false},
  {'$', KEY_4, true, false},
  {'%', KEY_5, true, false},
  {'&', KEY_6, true, false},
  {'/', KEY_7, true, false},
  {'(', KEY_8, true, false},
  {')', KEY_9, true, false},
  {'=', KEY_0, true, false},
  {'?', KEY_MINUS, true, false},
  {'^', KEY_EQUAL, false, false},
  {'@', KEY_Q, false, true},            // ALT GR + Q
  {'€', KEY_E, false, true},            // ALT GR + E
  {'[', KEY_è, false, true},
  {']', KEY_PLUS, false, true},
  {'\\', KEY_ù, false, true},
  {'|', KEY_BACKSLASH, false, true},
  {'#', KEY_3, false, true},
  {'\'', KEY_BACKSLASH, false, false},
};