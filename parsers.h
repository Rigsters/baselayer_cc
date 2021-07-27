#include "memory.h"
#include "token.h"

struct Node {
  char* name;
  u8 id;
  u8 clock_prio;
  char* ip_0;
  char* ip_1;
  char* port_loc;
  char* port_lan;
};


struct App {
  char* name;
  StringList nodes;
  char* state;
};


Node ParseNode(Tokenizer* tokenizer, StackAllocator* stack) {
  Token token;
  Node node = {};

  u8 fields_parsed = 0;
  bool pcheck = false;
  bool parsing = true;
  while (fields_parsed < 7) {
    token = GetToken(tokenizer);

    if (TokenEquals(&token, "Name")) {
      pcheck = RequireToken(tokenizer, NULL, TOK_COLON);
      assert(pcheck == true);

      token = GetToken(tokenizer);
      AllocStringField(&node.name, &token, stack);

      ++fields_parsed;
    } else 
    if (TokenEquals(&token, "ID")) {
      pcheck = RequireToken(tokenizer, NULL, TOK_COLON);
      assert(pcheck == true);

      // NOTE: assuming the format 0xab for hex number ab ..
      token = GetToken(tokenizer);
      node.id = strtol(token.text, NULL, 16);

      ++fields_parsed;
    } else 
    if (TokenEquals(&token, "clock_prio")) {
      pcheck = RequireToken(tokenizer, NULL, TOK_COLON);
      assert(pcheck == true);

      token = GetToken(tokenizer);
      node.clock_prio = (u8) atoi(token.text);

      ++fields_parsed;
    } else 
    if (TokenEquals(&token, "network_address_0")) {
      pcheck = RequireToken(tokenizer, NULL, TOK_COLON);
      assert(pcheck == true);

      token = GetToken(tokenizer);
      AllocStringField(&node.ip_0, &token, stack);

      ++fields_parsed;
    } else 
    if (TokenEquals(&token, "network_address_1")) {
      pcheck = RequireToken(tokenizer, NULL, TOK_COLON);
      assert(pcheck == true);

      token = GetToken(tokenizer);
      AllocStringField(&node.ip_1, &token, stack);

      ++fields_parsed;
    } else 
    if (TokenEquals(&token, "port_loc")) {
      pcheck = RequireToken(tokenizer, NULL, TOK_COLON);
      assert(pcheck == true);

      token = GetToken(tokenizer);
      AllocStringField(&node.port_loc, &token, stack);

      ++fields_parsed;
    } else 
    if (TokenEquals(&token, "port_lan")) {
      pcheck = RequireToken(tokenizer, NULL, TOK_COLON);
      assert(pcheck == true);

      token = GetToken(tokenizer);
      AllocStringField(&node.port_lan, &token, stack);

      ++fields_parsed;
    }
  }

  return node;
}

App ParseApp(Tokenizer* tokenizer, StackAllocator* stack) {
  Token token;
  App app = {};

  u8 fields_parsed = 0;
  bool pcheck = false;
  bool parsing = true;
  while (fields_parsed < 3) {
    token = GetToken(tokenizer);

    if (TokenEquals(&token, "Name")) {
      pcheck = RequireToken(tokenizer, NULL, TOK_COLON);
      assert(pcheck == true);

      token = GetToken(tokenizer);
      AllocStringField(&app.name, &token, stack);

      ++fields_parsed;
    } else 
    if (TokenEquals(&token, "Node")) {
      pcheck = RequireToken(tokenizer, NULL, TOK_COLON);
      assert(pcheck == true);
      pcheck = RequireToken(tokenizer, NULL, TOK_LSBRACK);
      assert(pcheck == true);

      ParseAllocCommaSeparatedListOfStrings(&app.nodes, tokenizer, stack);

      pcheck = RequireToken(tokenizer, NULL, TOK_RSBRACK);
      assert(pcheck == true);

      ++fields_parsed;
    } else 
    if (TokenEquals(&token, "State")) {
      pcheck = RequireToken(tokenizer, NULL, TOK_COLON);
      assert(pcheck == true);
      pcheck = RequireToken(tokenizer, NULL, TOK_LSBRACK);
      assert(pcheck == true);

      token = GetToken(tokenizer);
      AllocStringField(&app.state, &token, stack);

      pcheck = RequireToken(tokenizer, NULL, TOK_RSBRACK);
      assert(pcheck == true);

      ++fields_parsed;
    }
  }

  return app;
}

enum ParseMode {
  IDLE,
  NODES,
  APPS
};

void TestParseConfig() {
  char* text = LoadFile((char*) "config.yaml", true);
  if (text == NULL) {
    printf("could not load file");
    exit(1);
  }

  Tokenizer tokenizer = {};
  tokenizer.Init(text);
  ParseMode parsing_mode;

  // read the number of nodes / apps present
  u8 num_nodes = 0;
  u8 num_apps = 0;

  parsing_mode = IDLE;
  bool parsing = true;
  while (parsing) {
    Token token = GetToken(&tokenizer);
    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
      } break;

      case TOK_DASH: {
        if (parsing_mode == NODES) {
          ++num_nodes;
        } else
        if (parsing_mode == APPS) {
          ++num_apps;
        }
      } break;

      case TOK_IDENTIFIER: {
        if (TokenEquals(&token, "NODE") && RequireToken(&tokenizer, NULL, TOK_COLON)) {
          parsing_mode = NODES;
        } else 
        if (TokenEquals(&token, "APP") && RequireToken(&tokenizer, NULL, TOK_COLON)) {
          parsing_mode = APPS;
        }
      } break;

      default: {
      } break;
    }
  }

  StackAllocator stack(SIXTEEN_K);
  ArrayList nodes(stack.Alloc(sizeof(Node) * num_nodes), sizeof(Node));
  ArrayList apps(stack.Alloc(sizeof(App) * num_apps), sizeof(App));

  tokenizer.Init(text);
  parsing_mode = IDLE;
  parsing = true;
  while (parsing) {
    Token token = GetToken(&tokenizer);

    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
      } break;

      case TOK_DASH: {
        if (parsing_mode == NODES) {
          Node node = ParseNode(&tokenizer, &stack);
          nodes.Add(&node);
        } else
        if (parsing_mode == APPS) {
          App app = ParseApp(&tokenizer, &stack);
          apps.Add(&app);
        }

      } break;

      case TOK_IDENTIFIER: {
        if (TokenEquals(&token, "NODE") && RequireToken(&tokenizer, NULL, TOK_COLON)) {
          parsing_mode = NODES;
        } else
        if (TokenEquals(&token, "APP") && RequireToken(&tokenizer, NULL, TOK_COLON)) {
          parsing_mode = APPS;
        }
      } break;

      default: {
      } break;
    }
  }
}

/*
struct LinkedList {
  LinkedList *next;
  LinkedList *prev;
};
void ListInit(void* first) {
  ((LinkedList*) first)->next = (LinkedList*) first;
  ((LinkedList*) first)->prev = (LinkedList*) first;
}
void ListInsert(void* item, void* after) {
  ((LinkedList*) item)->next = ((LinkedList*) after)->next;
  ((LinkedList*) item)->prev = (LinkedList*) after;
  ((LinkedList*) after)->next->prev = (LinkedList*) item;
  ((LinkedList*) after)->next = (LinkedList*) item;
}
*/

struct InstrParam {
  char* name;
  char* type;
  char* defaultval;
};


ArrayListT<InstrParam> ParseInstrParams(Tokenizer *tokenizer, StackAllocator *stack) {

  // lookahead count how many pars there will be
  u32 count = 0;
  bool parstart_flag = true;
  Tokenizer resume = *tokenizer;
  bool parsing = true;
  while (parsing) {
    Token token = GetToken(tokenizer);
    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
        PrintLineError(tokenizer, &token, "Unexpected end of stream");
        exit(1);
      } break;

      case TOK_COMMA: {
        parstart_flag = true;
      } break;

      case TOK_RBRACK: {
        parsing = false;
      } break;

      case TOK_IDENTIFIER: {
        if (parstart_flag) {
          parstart_flag = false;
          ++count;
        }
      } break;

      default: {
      } break;
    }
  }
  *tokenizer = resume;

  ArrayListT<InstrParam> alst;
  alst.Init(stack->Alloc(sizeof(InstrParam) * count));

  parsing = true;
  while (parsing) {
    if (LookAheadNextToken(tokenizer, TOK_RBRACK)) {
      return alst;
    }
    Token token = GetToken(tokenizer);
    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
        assert(1 == 0);
      } break;

      case TOK_COMMA: {
        // empty
      } break;

      case TOK_RBRACK: {
        parsing = false;
      } break;

      case TOK_IDENTIFIER: {
        // TODO: ok I can parse, but how do I comminicate meaningful parsing errors to the user?

        InstrParam param = {};
        u32 count = LookAheadTokenCountOR(tokenizer, TOK_COMMA, TOK_RBRACK);

        if (count == 4) {
          // type varname = defval
          TokenValueToAllocAssertType(&token, TOK_IDENTIFIER, &param.type, stack);

          token = GetToken(tokenizer);
          TokenValueToAllocAssertType(&token, TOK_IDENTIFIER, &param.name, stack);

          GetToken(tokenizer);

          token = GetToken(tokenizer);
          if (token.type != TOK_FLOAT && token.type != TOK_INT && token.type != TOK_SCI && token.type != TOK_STRING) {
            tokenizer->at -= token.len;
            PrintLineError(tokenizer, &token, "Expected number or string");
            exit(1);
          }
          TokenValueToAlloc(&token, &param.defaultval, stack);
        }
        else if (count == 3) {
          // parname = defval
          TokenValueToAllocAssertType(&token, TOK_IDENTIFIER, &param.name, stack);

          GetToken(tokenizer);

          token = GetToken(tokenizer);
          if (token.type != TOK_FLOAT && token.type != TOK_INT && token.type != TOK_SCI && token.type != TOK_STRING) {
            PrintLineError(tokenizer, &token, "Expected number or string");
            exit(1);
          }
          TokenValueToAlloc(&token, &param.defaultval, stack);
        }
        else if (count == 2) {
          // type parname
          TokenValueToAllocAssertType(&token, TOK_IDENTIFIER, &param.type, stack);

          token = GetToken(tokenizer);
          TokenValueToAllocAssertType(&token, TOK_IDENTIFIER, &param.name, stack);
        }
        else if (count == 1) {
          // parname
          TokenValueToAllocAssertType(&token, TOK_IDENTIFIER, &param.defaultval, stack);
        }
        else {
          PrintLineError(tokenizer, &token, "Parameter ill defined");
          exit(1);
        }
        alst.Add(&param);
 
      } break;

      default: {
      } break;
    }
  }

  return alst;
}

char* CopyPercentBraceTextBlock(Tokenizer *tokenizer, StackAllocator *stack) {
  Token token;
  if (!RequireToken(tokenizer, &token, TOK_PERCENT)) exit(1);
  if (!RequireToken(tokenizer, &token, TOK_LBRACE)) exit(1);

  Tokenizer save = *tokenizer;
  char *text = NULL;

  while (true) {
    u32 dist = LookAheadTokenTextlen(tokenizer, TOK_PERCENT);
    if (dist == 0 && LookAheadNextToken(tokenizer, TOK_ENDOFSTREAM)) {
      PrintLineError(tokenizer, &token, "End of file reached");
      exit(0);
    }
    tokenizer->at += dist;
    if (!RequireToken(tokenizer, &token, TOK_PERCENT)) exit(0);
    if (!RequireToken(tokenizer, &token, TOK_RBRACE)) exit(0);

    u32 len = tokenizer->at - save.at - 2;
    text = (char*) stack->Alloc(len + 1);
    strncpy(text, save.at, len);
    text[len] = '\0';

    break;
  }
  return text;
}

struct DeclareDef {
  char * text;
};

struct InitializeDef {
  char * text;
};

struct FinalizeDef {
  char * text;
};

struct CompParam {
  char* name;
  char* value;
};

struct Vector3Strings {
  char* x;
  char* y;
  char* z;
};

struct CompDecl {
  char *type = NULL;
  char *name = NULL;
  char *extend = NULL;
  u32 split = 0;
  ArrayListT<CompParam> params;
  Vector3Strings at;
  Vector3Strings rot;
  char* at_relative = NULL; // value can be ABSOLUTE or a comp name
  char* rot_relative = NULL; // value must be a comp name
};

struct TraceDef {
  char * text;
  ArrayListT<CompDecl> comps;
};

struct InstrDef {
  char *name;
  ArrayListT<InstrParam> params;
  DeclareDef declare;
  InitializeDef init;
  TraceDef trace;
  FinalizeDef finalize;
};

struct StructMember {
  char *type = NULL;
  char *name = NULL;
  char *defval = NULL;
};


ArrayListT<StructMember> ParseStructMembers(char *text, StackAllocator *stack) {
  // NOTE: this one does not take a tokenizer, but builds its own, which decouples the proble somewhat
  Tokenizer _tokenizer_var;
  Tokenizer *tokenizer = &_tokenizer_var;
  tokenizer->Init(text);
  Tokenizer save = _tokenizer_var;

  // count the number of members by counting semi-colons
  bool parsing = true;
  u32 count = 0;
  while (parsing) {
    Token token = GetToken(tokenizer);

    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
      } break;

      case TOK_SEMICOLON: {
        ++count;
      } break;

      default: {
      } break;
    }
  }

  *tokenizer = save;
  ArrayListT<StructMember> lst;
  lst.Init(stack->Alloc(sizeof(StructMember) * count));
  Token token;

  while (true) {
    StructMember member;

    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(0);
    TokenValueToAlloc(&token, &member.type, stack);

    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(0);
    TokenValueToAlloc(&token, &member.name, stack);

    if (LookAheadNextToken(tokenizer, TOK_ASSIGN)) {
      GetToken(tokenizer); // skip the assign
      u32 len = LookAheadTokenTextlen(tokenizer, TOK_SEMICOLON);
      if (len == 0) {
        PrintLineError(tokenizer, &token, "Expected: ;");
        exit(0);
      }
      member.defval = (char*) stack->Alloc(len);
      strncpy(member.defval, tokenizer->at, len);

      tokenizer->at += len;
    }

    if (!RequireToken(tokenizer, &token, TOK_SEMICOLON)) exit(0);

    lst.Add(&member);

    // positive exit condition
    if (LookAheadNextToken(tokenizer, TOK_ENDOFSTREAM)) {
      break;
    }
  }

  return lst;
}

char* GetTextUntillEOSOrBlockClose(Tokenizer *tokenizer, StackAllocator *stack) {
  Tokenizer save = *tokenizer;

  // basic parsing pattern:
  bool parsing = true;
  while (parsing) {
    Token token = GetToken(tokenizer);

    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
      } break;

      case TOK_PERCENT: {
        if (LookAheadNextToken(tokenizer, TOK_RBRACE)) {
          tokenizer->at -= 1;
          parsing = false;
        }
      } break;

      case TOK_IDENTIFIER: {
        if (TokenEquals(&token, "END")) {
          tokenizer->at -= 3;
          parsing = false;
        }
      } break;

      default: {
      } break;
    }
  }

  u32 len = tokenizer->at - save.at;
  char *result = (char*) stack->Alloc(len + 1);
  strncpy(result, save.at, len);
  result[len] = '\0';
  return result;
}

u32 MinU(u32 a, u32 b) {
  if (a <= b) {
    return a;
  }
  else {
    return b;
  }
}

u32 MaxU(u32 a, u32 b) {
  if (a >= b) {
    return a;
  }
  else {
    return b;
  }
}

ArrayListT<CompParam> ParseCompParams(Tokenizer *tokenizer, StackAllocator *stack) {
  CompParam par;
  Token token;

  // find number of pars, check integrity
  Tokenizer save = *tokenizer;
  u32 count = 0;
  bool parsing = true;
  while (parsing) {
    Token token = GetToken(tokenizer);

    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
        PrintLineError(tokenizer, &token, "Unexpected end of stream");
      } break;

      case TOK_IDENTIFIER: {
        count = MaxU(count, 1);
      } break;

      case TOK_COMMA: {
        ++count;
        count = MaxU(count, 2);
      } break;

      case TOK_RBRACK: {
        parsing = false;
      } break;

      default: {
      } break;
    }
  }

  ArrayListT<CompParam> lst;
  lst.Init(stack->Alloc(sizeof(CompParam) * count));
  *tokenizer = save;

  // parse individual params
  for (int i = 0; i < count; i++) {
    CompParam par;

    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(0);
    TokenValueToAlloc(&token, &par.name, stack);

    if (!RequireToken(tokenizer, &token, TOK_ASSIGN)) exit(0);

    EatWhiteSpacesAndComments(tokenizer);
    u32 vallen = MinU(LookAheadTokenTextlen(tokenizer, TOK_COMMA), LookAheadTokenTextlen(tokenizer, TOK_RBRACK));
    if (vallen == 0) {
      PrintLineError(tokenizer, &token, "Expected param value.");
    }
    else {
      EatWhiteSpacesAndComments(tokenizer);
      token.text = tokenizer->at;
      token.len = vallen;
      TokenValueToAlloc(&token, &par.value, stack);
      lst.Add(&par);
      tokenizer->at += vallen;
    }

    if (LookAheadNextToken(tokenizer, TOK_COMMA)) {
      GetToken(tokenizer);
    }
  }

  return lst;
}

u32 Trim(char **src, u32 maxlen) { // trim whitespaces, sets *src = start and returns len (no null term)
  u32 idx = 0;
  char *at = *src;
  while ( IsWhitespace(*at) ) {
    ++at;
    ++idx;
  }

  *src = at;
  while ( !IsWhitespace(*at) && idx < maxlen) {
    ++at;
    ++idx;
  }
  return at - *src;
}

char* CopyAllocCharsUntillTok(TokenType token_type, Tokenizer *tokenizer, StackAllocator *stack) {
  char* result;
  u32 len = LookAheadTokenTextlen(tokenizer, token_type);
  len = Trim(&tokenizer->at, len);

  if (len == 1) { // safeguard against the value = zero case
    u8 alen = 3;
    result = (char*) stack->Alloc(alen + 1);
    result[0] = *tokenizer->at;
    result[1] = '.';
    result[2] = '0';
    result[3] = '\0';
  }
  else {
    result = (char*) stack->Alloc(len + 1);
    strncpy(result, tokenizer->at, len);
    result[len] = '\0';
  }

  tokenizer->at += len;
  return result;
}

Vector3Strings ParseVector3(Tokenizer *tokenizer, StackAllocator *stack) {
  Token token;
  Vector3Strings vect;

  EatWhiteSpacesAndComments(tokenizer);
  vect.x = CopyAllocCharsUntillTok(TOK_COMMA, tokenizer, stack);

  RequireToken(tokenizer, &token, TOK_COMMA);

  EatWhiteSpacesAndComments(tokenizer);
  vect.y = CopyAllocCharsUntillTok(TOK_COMMA, tokenizer, stack);

  RequireToken(tokenizer, &token, TOK_COMMA);

  EatWhiteSpacesAndComments(tokenizer);
  vect.z = CopyAllocCharsUntillTok(TOK_RBRACK, tokenizer, stack);

  return vect;
}

char* ParseAbsoluteRelative(Tokenizer *tokenizer, StackAllocator *stack) {
  char *result = NULL;
  Token token;
  if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(0);

  if (TokenEquals(&token, "ABSOLUTE")) {
    TokenValueToAlloc(&token, &result, stack);
  }
  else if (TokenEquals(&token, "RELATIVE")) {
    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(0);
    TokenValueToAlloc(&token, &result, stack);
  }
  else {
    PrintLineError(tokenizer, &token, "Expected RELATIVE [compname/PREVIOUS] or ABSOLUTE.");
    exit(0);
  }
  return result;
}


ArrayListT<CompDecl> ParseTraceComps(Tokenizer *tokenizer, StackAllocator *stack) {
  ArrayListT<CompDecl> result;

  Tokenizer save = *tokenizer;

  // basic parsing pattern:
  u32 count = 0;
  bool parsing = true;
  while (parsing) {
    Token token = GetToken(tokenizer);

    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
      } break;

      case TOK_IDENTIFIER: {
        if (TokenEquals(&token, "COMPONENT")) {
          ++count;
        }
        else if (TokenEquals(&token, "FINALIZE")) {
          parsing = false;
        }
        else if (TokenEquals(&token, "END")) {
          parsing = false;
        }
      } break;

      default: {
      } break;
    }
  }

  *tokenizer = save;
  Token token = {};
  ArrayListT<CompDecl> lst;
  lst.Init(stack->Alloc(sizeof(CompDecl) * count));
  for (int idx = 0; idx < count; ++idx) {
    if (LookAheadNextToken(tokenizer, TOK_ENDOFSTREAM)) {
      break;
    }
    else {
      CompDecl comp = {};

      // OPTIONAL split
      if (LookAheadNextToken(tokenizer, TOK_IDENTIFIER, "SPLIT")) {
        token = GetToken(tokenizer);
        comp.split = 1;
        if (LookAheadNextToken(tokenizer, TOK_INT)) {
          token = GetToken(tokenizer);
          char split[10];
          strncpy(split, token.text, token.len);
          comp.split = atoi(split);
          if (comp.split == 0) {
            PrintLineError(tokenizer, &token, "Expected a positive integer");
          }
        }
      }
      
      // declaration
      if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "COMPONENT")) exit(0);
      if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(0);
      TokenValueToAlloc(&token, &comp.name, stack);
      if (!RequireToken(tokenizer, &token, TOK_ASSIGN)) exit(0);
      if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(0);
      TokenValueToAlloc(&token, &comp.type, stack);

      // parameter values
      if (!RequireToken(tokenizer, &token, TOK_LBRACK)) exit(0);

      comp.params = ParseCompParams(tokenizer, stack);
      if (!RequireToken(tokenizer, &token, TOK_RBRACK)) exit(0);

      // location / AT
      if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "AT")) exit(0);
      RequireToken(tokenizer, &token, TOK_LBRACK);
      comp.at = ParseVector3(tokenizer, stack);
      RequireToken(tokenizer, &token, TOK_RBRACK);
      comp.at_relative = ParseAbsoluteRelative(tokenizer, stack);

      // OPTIONAL rotation / ROTATED
      if (LookAheadNextToken(tokenizer, TOK_IDENTIFIER, "ROTATED")) {
        token = GetToken(tokenizer);
        RequireToken(tokenizer, &token, TOK_LBRACK);
        comp.rot = ParseVector3(tokenizer, stack);
        RequireToken(tokenizer, &token, TOK_RBRACK);
        comp.rot_relative = ParseAbsoluteRelative(tokenizer, stack);
      }

      lst.Add(&comp);
    }
  }

  return lst;
}

void ParseInstrument(Tokenizer *tokenizer, StackAllocator *stack) {
  Token token;

  if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "DEFINE")) exit(1);
  if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "INSTRUMENT")) exit(1);
  if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(1);

  // instr header
  InstrDef instr;
  {
    TokenValueToAlloc(&token, &instr.name, stack);

    if (!RequireToken(tokenizer, NULL, TOK_LBRACK)) exit(1);

    instr.params = ParseInstrParams(tokenizer, stack);

    if (!RequireToken(tokenizer, NULL, TOK_RBRACK)) exit(1);

    printf("instr name:\n%s\n\n", instr.name);
    printf("instr params:\n");
    for (int i = 0; i < instr.params.len; i++) {
      InstrParam* p = instr.params.At(i);
      printf("  %s %s = %s\n", p->type, p->name, p->defaultval);
    }
    printf("\n");
  }

  // declare
  if (LookAheadNextToken(tokenizer, TOK_IDENTIFIER, "DECLARE")) {
    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "DECLARE")) exit(1);

    DeclareDef declare;
    instr.declare = declare;
    instr.declare.text = CopyPercentBraceTextBlock(tokenizer, stack);


    ArrayListT<StructMember> lst = ParseStructMembers(instr.declare.text, stack);
    
    printf("declare members:\n");
    for (int i = 0; i < lst.len; ++i) {
      StructMember *memb = lst.At(i);
      printf("  %s %s = %s\n", memb->type, memb->name, memb->defval);
    }
    printf("\n");
  }

  // initialize
  if (LookAheadNextToken(tokenizer, TOK_IDENTIFIER, "INITIALIZE")) {
    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "INITIALIZE")) exit(1);

    InitializeDef init;
    instr.init = init;
    instr.init.text = CopyPercentBraceTextBlock(tokenizer, stack);

    printf("init text:%s\n\n", instr.init.text);
  }

  // trace
  {
    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "TRACE")) exit(1);

    TraceDef trace;
    instr.trace = trace;
    Tokenizer save = *tokenizer;
    instr.trace.text = GetTextUntillEOSOrBlockClose(tokenizer, stack);
    *tokenizer = save;
    trace.comps = ParseTraceComps(tokenizer, stack);

    printf("components:\n");
    auto lst = trace.comps;
    for (int i = 0; i < lst.len; ++i) {
      CompDecl *comp = lst.At(i);
      printf("  type: %s\n", comp->type);
      printf("  name %s\n", comp->name);
      printf("  params:\n");
      auto lstp = comp->params;
      for (int i = 0; i < lstp.len; ++i) {
        CompParam *param = lstp.At(i);
        printf("    name: %s\n", param->name);
        printf("    value %s\n", param->value);
      }
      printf("  at:      (%s, %s, %s) %s\n", comp->at.x, comp->at.y, comp->at.z, comp->at_relative);
      printf("  rotated: (%s, %s, %s) %s\n", comp->rot.x, comp->rot.y, comp->rot.z, comp->rot_relative);
    }
    printf("\n");

    //printf("trace text: \n%s\n\n", instr.trace.text);
  }

  // finalize
  if (LookAheadNextToken(tokenizer, TOK_IDENTIFIER, "FINALIZE")) {
    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "FINALIZE")) exit(1);

    FinalizeDef finalize;
    instr.finalize = finalize;
    instr.finalize.text = CopyPercentBraceTextBlock(tokenizer, stack);

    printf("finalize text:%s\n\n", instr.finalize.text);
  }


  /*
  // basic parsing pattern:
  bool parsing = true;
  while (parsing) {
    Token token = GetToken(&tokenizer);

    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
      } break;

      default: {
      } break;
    }
  }
  */
}


void TestParseMcStas(int argc, char **argv) {
  //char *filename = (char*) "PSI_DMC.instr";
  char *filename = (char*) "PSI.instr";
  char *text = LoadFile(filename, true);
  if (text == NULL) {
      printf("could not load file %s\n", filename);
      exit(1);
  }
  printf("parsing file %s\n\n", filename);

  StackAllocator stack(MEGABYTE);

  Tokenizer tokenizer = {};
  tokenizer.Init(text);

  /*
  // DB TEST newline capture
  bool parsing = true;
  u32 count = 0;
  while (count < 20) {
    Token token = GetToken(&tokenizer);

    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
      } break;

      default: {
        ++count;
        //PrintLineNo(&tokenizer);
      } break;
    }
  }
  printf("end\n");
  exit(0);
  */

  ParseInstrument(&tokenizer, &stack);
}
