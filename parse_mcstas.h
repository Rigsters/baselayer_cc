#include "memory.h"
#include "token.h"


//
// parsing C structs


struct StructMember {
  char *type = NULL;
  char *name = NULL;
  char *defval = NULL;
};

ArrayListT<StructMember> ParseStructMembers(Tokenizer *tokenizer, StackAllocator *stack) {
  ArrayListT<StructMember> lst;
  u32 count = CountTokenSeparatedStuff(tokenizer->at, TOK_SEMICOLON, TOK_ENDOFSTREAM, TOK_UNKNOWN, TOK_COMMA);
  lst.Init(stack->Alloc(sizeof(StructMember) * count));

  StructMember member;
  bool parsing = true;
  while (parsing) {
    Token token = GetToken(tokenizer);
    switch ( token.type ) {
      case TOK_ENDOFSTREAM: {
        parsing = false;
      } break;

      case TOK_RPERBRACE: {
        parsing = false;
        tokenizer->at -= token.len;
      } break;

      case TOK_SEMICOLON: {
        lst.Add(&member);
        member = {};
      } break;

      case TOK_COMMA: {
        lst.Add(&member);
        member.name = NULL;
        member.defval = NULL;
      } break;

      case TOK_ASSIGN: {
        if (member.type != NULL && member.name != NULL) {
          // parse everything until next comma / semicolon
          token = GetToken(tokenizer);
          if (token.type == TOK_ENDOFSTREAM) {
            PrintLineError(tokenizer, &token, "Unexpected end of stream:");
            exit(1);
          }
          else if (token.type == TOK_SEMICOLON || token.type == TOK_COMMA) {
            PrintLineError(tokenizer, &token, "Expected default value");
            exit(1);
          }
          Token block = token;
          u32 tcount = LookAheadTokenCountOR(tokenizer, TOK_SEMICOLON, TOK_COMMA);
          for (int i = 1; i < tcount; ++i) {
            token = GetToken(tokenizer);
            block.len += token.len;
          }
          AllocTokenValue(&member.defval, &block, stack);
        }
        else {
          PrintLineError(tokenizer, &token, "Unexpected assign:");
          exit(1);
        }
      } break;

      case TOK_IDENTIFIER: {
        if (member.type != NULL && member.name != NULL) {
          PrintLineError(tokenizer, &token, "Expected assign, comma or semicolon");
          exit(1);
        }
        if (member.type != NULL && member.name == NULL) {
          AllocTokenValue(&member.name, &token, stack);
        }
        else if (member.type == NULL && member.name == NULL) {
          AllocTokenValue(&member.type, &token, stack);
        }
      } break;

      default: {
      } break;
    }
  }

  return lst;
}


//
// parsing mcstas .instr files:


struct InstrParam {
  char* name = NULL;
  char* type = NULL;
  char* defaultval = NULL;
};

struct DeclareDef {
  char *text = NULL;
  ArrayListT<StructMember> decls;
};

struct UservarsDef {
  char *text = NULL;
  ArrayListT<StructMember> decls;
};

struct InitializeDef {
  char *text = NULL;
};

struct FinalizeDef {
  char *text = NULL;
};

struct CompParam {
  char *name = NULL;
  char *value = NULL;
};

struct Vector3Strings {
  char *x;
  char *y;
  char *z;
};

struct CompDecl {
  char *type = NULL;
  char *name = NULL;
  char *extend = NULL;
  char *copy = NULL;
  char *when = NULL;
  bool removable = false;
  u32 split = 0;
  ArrayListT<CompParam> params;
  Vector3Strings at;
  Vector3Strings rot;
  char *at_relative = NULL; // value can be ABSOLUTE or a comp name
  char *rot_relative = NULL; // value must be a comp name
  char *percent_include = NULL; // NOTE: includes aren't components
};

struct TraceDef {
  char *text = NULL;
  ArrayListT<CompDecl> comps;
};

struct InstrDef {
  char *name = NULL;
  ArrayListT<InstrParam> params;
  DeclareDef declare;
  UservarsDef uservars;
  InitializeDef init;
  TraceDef trace;
  FinalizeDef finalize;
};

ArrayListT<InstrParam> ParseInstrParams(Tokenizer *tokenizer, StackAllocator *stack) {
  u32 count = CountTokenSeparatedStuff(tokenizer->at, TOK_COMMA, TOK_RBRACK);

  ArrayListT<InstrParam> alst;
  alst.Init(stack->Alloc(sizeof(InstrParam) * count));

  bool parsing = true;
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

      } break;

      case TOK_RBRACK: {
        parsing = false;
      } break;

      case TOK_IDENTIFIER: {
        InstrParam param = {};
        u32 count = LookAheadTokenCountOR(tokenizer, TOK_COMMA, TOK_RBRACK);

        if (count == 4) {
          // type varname = defval
          AllocTokenValueAssertType(&param.type, &token, TOK_IDENTIFIER, stack);

          token = GetToken(tokenizer);
          AllocTokenValueAssertType(&param.name, &token, TOK_IDENTIFIER, stack);

          GetToken(tokenizer);

          token = GetToken(tokenizer);
          if (token.type != TOK_FLOAT && token.type != TOK_INT && token.type != TOK_SCI && token.type != TOK_STRING) {
            tokenizer->at -= token.len;
            PrintLineError(tokenizer, &token, "Expected number or string");
            exit(1);
          }
          AllocTokenValue(&param.defaultval, &token, stack);
        }
        else if (count == 3) {
          // parname = defval
          AllocTokenValueAssertType(&param.name, &token, TOK_IDENTIFIER, stack);

          GetToken(tokenizer);

          token = GetToken(tokenizer);
          if (token.type != TOK_FLOAT && token.type != TOK_INT && token.type != TOK_SCI && token.type != TOK_STRING) {
            PrintLineError(tokenizer, &token, "Expected number or string");
            exit(1);
          }
          AllocTokenValue(&param.defaultval, &token, stack);
        }
        else if (count == 2) {
          // type parname
          AllocTokenValueAssertType(&param.type, &token, TOK_IDENTIFIER, stack);

          token = GetToken(tokenizer);
          AllocTokenValueAssertType(&param.name, &token, TOK_IDENTIFIER, stack);
        }
        else if (count == 1) {
          // parname
          AllocTokenValueAssertType(&param.defaultval, &token, TOK_IDENTIFIER, stack);
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

char* CopyBracketedTextBlock(Tokenizer *tokenizer, TokenType type_start, TokenType type_end, bool restore_tokenizer, StackAllocator *stack) {
  Tokenizer save = *tokenizer;

  if (type_start != TOK_UNKNOWN) {
    if (!RequireToken(tokenizer, NULL, type_start)) exit(1);
  }

  char *text_start = tokenizer->at;
  LTrim(&text_start);

  Token token = {};
  token.type = TOK_UNKNOWN;
  char *text = NULL;

  u32 dist = LookAheadLenUntilToken(tokenizer, type_end);
  if (dist == 0 && LookAheadNextToken(tokenizer, TOK_ENDOFSTREAM)) {
    PrintLineError(tokenizer, &token, "End of file reached");
    exit(1);
  }
  else if (dist == 1) {
    token = GetToken(tokenizer);
  }
  else if (dist > 1) {
    while (token.type != type_end) {
      token = GetToken(tokenizer);
    }

    u32 len_untrimmed = tokenizer->at - text_start - token.len;
    u32 len = RTrimText(text_start, len_untrimmed);

    text = (char*) stack->Alloc(len + 1);
    strncpy(text, text_start, len);
    text[len] = '\0';
  }

  if (restore_tokenizer == true) {
    *tokenizer = save;
  }
  return text;
}

ArrayListT<CompParam> ParseCompParams(Tokenizer *tokenizer, StackAllocator *stack) {
  u32 count = CountTokenSeparatedStuff(tokenizer->at, TOK_COMMA, TOK_RBRACK);

  ArrayListT<CompParam> lst;
  lst.Init(stack->Alloc(sizeof(CompParam) * count));
  Token token;

  for (int i = 0; i < count; i++) {
    CompParam par;

    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(1);
    AllocTokenValue(&par.name, &token, stack);

    if (!RequireToken(tokenizer, &token, TOK_ASSIGN)) exit(1);

    char* expr_start = tokenizer->at;
    if (!ParseExpression(tokenizer, &token)) {
      PrintLineError(tokenizer, &token, "Expected param value.");
      exit(1);
    }
    else {
      AllocTokenValue(&par.value, &token, stack);
      lst.Add(&par);
    }

    if (LookAheadNextToken(tokenizer, TOK_COMMA)) {
      GetToken(tokenizer);
    }
  }

  return lst;
}

char* CopyAllocCharsUntillTok(Tokenizer *tokenizer, TokenType token_type, StackAllocator *stack) {
  char* result;

  EatWhiteSpacesAndComments(tokenizer);

  u32 len_raw = LookAheadLenUntilToken(tokenizer, token_type);
  u32 len = RTrimText(tokenizer->at, len_raw);

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

  IncTokenizerUntilAtToken(tokenizer, token_type);
  return result;
}

Vector3Strings ParseVector3(Tokenizer *tokenizer, StackAllocator *stack) {
  Token token;
  Vector3Strings vect;

  if (ParseExpression(tokenizer, &token)) {
    AllocTokenValue(&vect.x, &token, stack);
  }
  else {
    PrintLineError(tokenizer, &token, "Expected value");
    exit(1);
  }
  if (!RequireToken(tokenizer, &token, TOK_COMMA)) exit(1);

  if (ParseExpression(tokenizer, &token)) {
    AllocTokenValue(&vect.y, &token, stack);
  }
  else {
    PrintLineError(tokenizer, &token, "Expected value");
    exit(1);
  }
  if (!RequireToken(tokenizer, &token, TOK_COMMA)) exit(1);

  if (ParseExpression(tokenizer, &token)) {
    AllocTokenValue(&vect.z, &token, stack);
  }
  else {
    PrintLineError(tokenizer, &token, "Expected value");
    exit(1);
  }
  if (!RequireToken(tokenizer, &token, TOK_RBRACK)) exit(1);

  return vect;
}

char* ParseAbsoluteRelative(Tokenizer *tokenizer, StackAllocator *stack) {
  char *result = NULL;
  Token token;
  if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(1);

  if (TokenEquals(&token, "ABSOLUTE") || TokenEquals(&token, "absolute")) {
    AllocTokenValue(&result, &token, stack);
  }
  else if (TokenEquals(&token, "RELATIVE") || TokenEquals(&token, "relative")) {
    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(1);
    AllocTokenValue(&result, &token, stack);
  }
  else {
    PrintLineError(tokenizer, &token, "Expected RELATIVE [compname/PREVIOUS] or ABSOLUTE.");
    exit(1);
  }
  return result;
}

ArrayListT<CompDecl> ParseTraceComps(Tokenizer *tokenizer, StackAllocator *stack) {
  ArrayListT<CompDecl> result;
  Tokenizer save = *tokenizer;
  Token token = {};

  // get number of comps
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

  // prepare
  *tokenizer = save;
  ArrayListT<CompDecl> lst;
  lst.Init(stack->Alloc(sizeof(CompDecl) * count));

  // parse comps
  for (int idx = 0; idx < count; ++idx) {
    if (LookAheadNextToken(tokenizer, TOK_ENDOFSTREAM)) {
      break;
    }
    else {
      CompDecl comp = {};

      // removable [OPTIONAL]
      if (RequireToken(tokenizer, &token, TOK_PERCENT, NULL, false)) {
        if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "include")) exit(1);
        if (!RequireToken(tokenizer, &token, TOK_STRING)) exit(1);
        AllocTokenValue(&comp.percent_include, &token, stack);
        continue;
      }

      // split [OPTIONAL]
      if (RequireToken(tokenizer, &token, TOK_IDENTIFIER, "SPLIT", false)) {
        comp.split = 1;
        if (RequireToken(tokenizer, &token, TOK_INT, NULL, false)) {
          char split[10];
          strncpy(split, token.text, token.len);
          comp.split = atoi(split);
          if (comp.split == 0) {
            PrintLineError(tokenizer, &token, "Expected a positive integer");
            exit(1);
          }
        }
      }

      // removable [OPTIONAL]
      if (RequireToken(tokenizer, &token, TOK_IDENTIFIER, "REMOVABLE", false)) {
        comp.removable = true;
      }

      // declaration
      if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "COMPONENT")) exit(1);
      if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(1);
      AllocTokenValue(&comp.name, &token, stack);
      if (!RequireToken(tokenizer, &token, TOK_ASSIGN)) exit(1);
      if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(1);
      // copy [OPTIONAL]
      if (TokenEquals(&token, "COPY")) {
        if (!RequireToken(tokenizer, &token, TOK_LBRACK)) exit(1);
        if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(1);
        // TODO: whenever there is a copy, the type must be set by a post-processing step
        AllocTokenValue(&comp.copy, &token, stack);
        if (!RequireToken(tokenizer, &token, TOK_RBRACK)) exit(1);
      }
      else {
        AllocTokenValue(&comp.type, &token, stack);
      }

      // params
      if (!RequireToken(tokenizer, &token, TOK_LBRACK)) exit(1);
      comp.params = ParseCompParams(tokenizer, stack);
      if (!RequireToken(tokenizer, &token, TOK_RBRACK)) exit(1);

      // WHEN
      if (RequireToken(tokenizer, &token, TOK_IDENTIFIER, "WHEN", false)) {
        // TODO: this function should respect the use of nested tok_start and tok_end usage inside the text block
        comp.when = CopyBracketedTextBlock(tokenizer, TOK_LBRACK, TOK_RBRACK, false, stack);
      }

      // location / AT
      if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "AT")) exit(1);
      RequireToken(tokenizer, &token, TOK_LBRACK, NULL, false);
      comp.at = ParseVector3(tokenizer, stack);
      RequireToken(tokenizer, &token, TOK_RBRACK, NULL, false);
      comp.at_relative = ParseAbsoluteRelative(tokenizer, stack);

      // rotation / ROTATED [OPTIONAL]
      if (RequireToken(tokenizer, NULL, TOK_IDENTIFIER, "ROTATED", false)) {
        RequireToken(tokenizer, &token, TOK_LBRACK, NULL, false);

        comp.rot = ParseVector3(tokenizer, stack);
        RequireToken(tokenizer, &token, TOK_RBRACK, NULL, false);
        comp.rot_relative = ParseAbsoluteRelative(tokenizer, stack);
      }

      // EXTEND [OPTIONAL]
      if (RequireToken(tokenizer, &token, TOK_IDENTIFIER, "EXTEND", false)) {
        comp.extend = CopyBracketedTextBlock(tokenizer, TOK_LPERBRACE, TOK_RPERBRACE, false, stack);
      }

      lst.Add(&comp);
    }
  }

  return lst;
}

InstrDef ParseInstrument(Tokenizer *tokenizer, StackAllocator *stack) {
  Token token;

  if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "DEFINE")) exit(1);
  if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "INSTRUMENT")) exit(1);
  if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER)) exit(1);

  // instr header
  InstrDef instr;
  {
    AllocTokenValue(&instr.name, &token, stack);
    if (!RequireToken(tokenizer, NULL, TOK_LBRACK)) exit(1);
    instr.params = ParseInstrParams(tokenizer, stack);
    if (!RequireToken(tokenizer, NULL, TOK_RBRACK)) exit(1);
  }

  // declare
  if (RequireToken(tokenizer, &token, TOK_IDENTIFIER, "DECLARE", false)) {
    instr.declare.text = CopyBracketedTextBlock(tokenizer, TOK_LPERBRACE, TOK_RPERBRACE, true, stack);
    if (!RequireToken(tokenizer, &token, TOK_LPERBRACE)) exit(1);
    instr.declare.decls = ParseStructMembers(tokenizer, stack);
    if (!RequireToken(tokenizer, &token, TOK_RPERBRACE)) exit(1);
  }

  // uservars
  if (RequireToken(tokenizer, &token, TOK_IDENTIFIER, "USERVARS", false)) {
    instr.uservars.text = CopyBracketedTextBlock(tokenizer, TOK_LPERBRACE, TOK_RPERBRACE, true, stack);
    if (!RequireToken(tokenizer, &token, TOK_LPERBRACE)) exit(1);
    instr.uservars.decls = ParseStructMembers(tokenizer, stack);
    if (!RequireToken(tokenizer, &token, TOK_RPERBRACE)) exit(1);
  }

  // initialize
  if (RequireToken(tokenizer, &token, TOK_IDENTIFIER, "INITIALIZE", false)) {
    instr.init.text = CopyBracketedTextBlock(tokenizer, TOK_LPERBRACE, TOK_RPERBRACE, false, stack);
  }

  // trace
  {
    if (!RequireToken(tokenizer, &token, TOK_IDENTIFIER, "TRACE")) exit(1);
    instr.trace.comps = ParseTraceComps(tokenizer, stack);
  }

  // finalize
  if (RequireToken(tokenizer, &token, TOK_IDENTIFIER, "FINALLY", false)) {
    instr.finalize.text = CopyBracketedTextBlock(tokenizer, TOK_LPERBRACE, TOK_RPERBRACE, false, stack);
  }

  return instr;
}

void PrintInstrumentParse(InstrDef instr) {
  printf("instr name: %s\n", instr.name);
  printf("instr params:\n");
  for (int i = 0; i < instr.params.len; i++) {
    InstrParam* p = instr.params.At(i);
    printf("  %s %s = %s\n", p->type, p->name, p->defaultval);
  }
  printf("\n");

  printf("declare members:\n");
  for (int i = 0; i < instr.declare.decls.len; ++i) {
    StructMember *memb = instr.declare.decls.At(i);
    printf("  %s %s = %s\n", memb->type, memb->name, memb->defval);
  }
  printf("\n");

  printf("uservars members:\n");
  for (int i = 0; i < instr.uservars.decls.len; ++i) {
    StructMember *memb = instr.uservars.decls.At(i);
    printf("  %s %s = %s\n", memb->type, memb->name, memb->defval);
  }
  printf("\n");

  printf("init text:\n%s\n\n", instr.init.text);

  printf("components:\n");
  for (int i = 0; i < instr.trace.comps.len; ++i) {
    CompDecl *comp = instr.trace.comps.At(i);
    printf("\n  type: %s\n", comp->type);
    printf("\n  copy: %s\n", comp->copy);
    printf("  name: %s\n", comp->name);
    printf("  split: %d\n", comp->split);
    printf("  removable: %d\n", comp->removable);
    printf("  params:\n");
    auto lstp = comp->params;
    for (int i = 0; i < lstp.len; ++i) {
      CompParam *param = lstp.At(i);
      printf("    %s = %s\n", param->name, param->value);
    }
    printf("  when: %s\n", comp->when);
    printf("  at:      (%s, %s, %s) %s\n", comp->at.x, comp->at.y, comp->at.z, comp->at_relative);
    printf("  rotated: (%s, %s, %s) %s\n", comp->rot.x, comp->rot.y, comp->rot.z, comp->rot_relative);
    printf("  extend:\n%s\n", comp->extend);
  }
  printf("\n");

  printf("finalize text:\n%s\n\n", instr.finalize.text);
}

void TestParseMcStasInstr(int argc, char **argv) {
  StackAllocator stack(MEGABYTE);

  char *filename = (char*) "PSI.instr";
  char *text = LoadFile(filename, true, &stack);
  if (text == NULL) {
      printf("could not load file %s\n", filename);
      exit(1);
  }
  printf("parsing file %s\n\n", filename);


  Tokenizer tokenizer = {};
  tokenizer.Init(text);

  InstrDef instr = ParseInstrument(&tokenizer, &stack);

  PrintInstrumentParse(instr);
}

void TestParseMcStasInstrExamplesFolder(int argc, char **argv) {
  char *folder = (char*) "/usr/share/mcstas/3.0-dev/examples";
  StackAllocator stack_files(MEGABYTE);
  StackAllocator stack_work(MEGABYTE);

  ArrayListT<char*> filepaths = GetFilesInFolderPaths(folder, &stack_files);

  //for (int i = 0; i < filepaths.len; ++i) {
  for (int i = 12; i < 13; ++i) {
    stack_work.Clear();

    char *filename = *filepaths.At(i);
    char *text = LoadFile(filename, false, &stack_files);
    if (text == NULL) {
      continue;
    }

    printf("parsing: %s  ", filename);

    Tokenizer tokenizer = {};
    tokenizer.Init(text);

    InstrDef instr = ParseInstrument(&tokenizer, &stack_work);

    PrintInstrumentParse(instr);

    // TODO: print instrdef

    printf("  %s\n", instr.name);
  }

}