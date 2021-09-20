/*
** $Id: luac.c,v 1.44a 2003/04/07 20:34:20 lhf Exp $
** Lua compiler (saves bytecodes to files; also list bytecodes)
** See Copyright Notice in lua.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>

#include "lua.h"
#include "lauxlib.h"

#include "lfunc.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lstring.h"
#include "lundump.h"

#include "resources.h"
#include "pdb.h"

#ifndef LUA_DEBUG
#define luaB_opentests(L)
#endif

#ifndef PROGNAME
#define PROGNAME	"plua2c"	/* program name */
#endif

#define	OUTPUT		"PluaApp.prc"	/* default output file */

static int listing=0;			/* list bytecodes? */
static int dumping=1;			/* dump bytecodes? */
static int stripping=0;			/* strip debug information? */
static char Output[]={ OUTPUT };	/* default output file name */
static char* output=Output;	/* output file name */
static char* progname=PROGNAME;	/* actual program name */

#define PLUA_COPYRIGHT	"Copyright (C) Marcio Migueletto de Andrade"
#define	APPNAME		"PluaApp"
#define	CID		"PlAp"
#define	VER		"1.0"

#define codeType	"LuaP"

static int islib=0;
static int notitle=0;
static char Appname[]={ APPNAME };
static char* appname=Appname;
static char Cid[]={ CID };
static char* cid=Cid;
static char Ver[]={ VER };
static char* ver=Ver;

static unsigned char buffer[65536];
static int count = 0;

static void fatal(const char *fmt, ...)
{
 va_list argp;

 fprintf(stderr,"%s: ", progname);
 va_start(argp, fmt);
 vfprintf(stderr, fmt, argp);
 va_end(argp);
 fprintf(stderr,"\n");

 exit(EXIT_FAILURE);
}

static void usage(const char* message, const char* arg)
{
 if (message!=NULL)
 {
  fprintf(stderr,"%s: ",progname); fprintf(stderr,message,arg); fprintf(stderr,"\n");
 }
 fprintf(stderr,
 "usage: %s [options] filenames.  Available options are:\n"
 "  -l       list\n"
 "  -o name  output to file `name' (default is \"" OUTPUT "\")\n"
 "  -p       parse only\n"
 "  -s       strip debug information\n"
 "  -v       show version information\n"
 "  -lib     compiles a library instead of a full application\n"
 "  -nt      application will not have a title frame\n"
 "  -name    application name in launcher\n"
 "  -cid     application creator ID as a four letter string\n"
 "  -ver     application version string\n"
 "  --       stop handling options\n",
 progname);
 exit(EXIT_FAILURE);
}

#define	IS(s)	(strcmp(argv[i],s)==0)

static int doargs(int argc, char* argv[])
{
 int i;
 if (argv[0]!=NULL && *argv[0]!=0) progname=argv[0];
 for (i=1; i<argc; i++)
 {
  if (*argv[i]!='-')			/* end of options; keep it */
   break;
  else if (IS("--"))			/* end of options; skip it */
  {
   ++i;
   break;
  }
  else if (IS("-l"))			/* list */
   listing=1;
  else if (IS("-o"))			/* output file */
  {
   output=argv[++i];
   if (output==NULL || *output==0) usage("`-o' needs argument",NULL);
  }
  else if (IS("-p"))			/* parse only */
   dumping=0;
  else if (IS("-s"))			/* strip debug information */
   stripping=1;
  else if (IS("-v"))			/* show version */
  {
   printf("Plua %s  %s\n", (char *)PluaVersion, PLUA_COPYRIGHT);
   printf("%s  %s\n",LUA_VERSION,LUA_COPYRIGHT);
#ifdef EXTRA_CREDITS
   printf("%s\n", EXTRA_CREDITS);
#endif
   if (argc==2) exit(EXIT_SUCCESS);
  }
  else if (IS("-lib"))
   islib=1;
  else if (IS("-nt"))
   notitle=1;
  else if (IS("-name"))
  {
   appname=argv[++i];
   if (appname==NULL || *appname==0) usage("`-name' needs argument",NULL);
  }
  else if (IS("-cid"))
  {
   cid=argv[++i];
   if (cid==NULL || *cid==0) usage("`-cid' needs argument",NULL);
  }
  else if (IS("-ver"))
  {
   ver=argv[++i];
   if (ver==NULL || *ver==0) usage("`-ver' needs argument",NULL);
  }
  else					/* unknown option */
   usage("unrecognized option `%s'",argv[i]);
 }
 if (i==argc && (listing || !dumping))
 {
  dumping=0;
  argv[--i]=Output;
 }
 return i;
}

static Proto* toproto(lua_State* L, int i)
{
 const Closure* c=(const Closure*)lua_topointer(L,i);
 return c->l.p;
}

static Proto* combine(lua_State* L, int n)
{
 if (n==1)
  return toproto(L,-1);
 else
 {
  int i,pc=0;
  Proto* f=luaF_newproto(L);
  f->source=luaS_newliteral(L,"=(" PROGNAME ")");
  f->maxstacksize=1;
  f->p=luaM_newvector(L,n,Proto*);
  f->sizep=n;
  f->sizecode=2*n+1;
  f->code=luaM_newvector(L,f->sizecode,Instruction);
  for (i=0; i<n; i++)
  {
   f->p[i]=toproto(L,i-n);
   f->code[pc++]=CREATE_ABx(OP_CLOSURE,0,i);
   f->code[pc++]=CREATE_ABC(OP_CALL,0,1,1);
  }
  f->code[pc++]=CREATE_ABC(OP_RETURN,0,1,0);
  return f;
 }
}

static void strip(lua_State* L, Proto* f)
{
 int i,n=f->sizep;
 luaM_freearray(L, f->lineinfo, f->sizelineinfo, int);
 luaM_freearray(L, f->locvars, f->sizelocvars, struct LocVar);
 luaM_freearray(L, f->upvalues, f->sizeupvalues, TString *);
 f->lineinfo=NULL; f->sizelineinfo=0;
 f->locvars=NULL;  f->sizelocvars=0;
 f->upvalues=NULL; f->sizeupvalues=0;
 f->source=luaS_newliteral(L,"=(none)");
 for (i=0; i<n; i++) strip(L,f->p[i]);
}

static int writer(lua_State* L, const void* p, size_t size, void* u)
{
 unsigned char *buf = u;
 memcpy(&buf[count], p, size);
 count += size;
  return 1;
}

int main(int argc, char* argv[])
{
 lua_State* L;
 Proto* f;
 int i=doargs(argc,argv);
 argc-=i; argv+=i;
 if (argc<=0) usage("no input files given",NULL);
 L=lua_open();
 luaB_opentests(L);
 for (i=0; i<argc; i++)
 {
  if (!strstr(argv[i], ".lua"))
    break;
  if (luaL_loadfile(L,argv[i])!=0) fatal(lua_tostring(L,-1));
 }
 f=combine(L,i);
 if (listing) luaU_print(f);
 if (dumping)
 {
  pdb_t *pdb;
  int tAIB, tAIBsmall;
  unsigned char scrt[4];

  if (stripping) strip(L,f);
  lua_lock(L);
  count=0;
  luaU_dump(L,f,writer,buffer);
  lua_unlock(L);

  pdb = pdb_open(output, appname, cid, islib ? "LuaL" : "appl", 0x0009);

  if (!islib) {
    pdb_addres(pdb, code0000, sizeof(code0000), "code", 0);
    pdb_addres(pdb, code0001, sizeof(code0001), "code", 1);
    pdb_addres(pdb, data0000, sizeof(data0000), "data", 0);
    pdb_addres(pdb, pref0000, sizeof(pref0000), "pref", 0);
  }
  pdb_addres(pdb, buffer, count, codeType, 1);
  pdb_addres(pdb, (UInt8 *)ver, strlen(ver)+1, "tver", 0x0001);
  tAIB = 0;
  tAIBsmall = 0;

  if (!islib && notitle) {
    scrt[0] = scrt[1] = scrt[2] = scrt[3];
    pdb_addres(pdb, scrt, sizeof(scrt), "ScrT", 1);
  }

  for (i=0; i<argc; i++)
  {
   int n, fd;
   unsigned short id;
   char *s;

   if (!strstr(argv[i], ".bin"))
     continue;

   s = argv[i];
   if (strlen(s) != 12 || strcmp(&s[8], ".bin") ||
       !isalpha(s[0]) || !isalpha(s[1]) ||
       !isalpha(s[2]) || !isalpha(s[3]) ||
       !isxdigit(s[4]) || !isxdigit(s[5]) ||
       !isxdigit(s[6]) || !isxdigit(s[7]) ||
       !strncmp(s, "code", 4) || !strncmp(s, codeType, 4) ||
       !strncmp(s, "rloc", 4) || !strncmp(s, "pref", 4) ||
       !strncmp(s, "data", 4))
     fatal("Invalid resource \"%s\"", s);

   if ((fd = open(s, O_RDONLY | O_BINARY)) == -1)
    fatal("Can not open resource \"%s\"", s);

   if ((n = read(fd, buffer, sizeof(buffer))) < 0)
    fatal("Can not read resource \"%s\"", s);

   close(fd);

   if (n > 0) {
     id = (unsigned short)strtol(&s[4], NULL, 16);
     pdb_addres(pdb, buffer, n, s, id);
     if (!strncmp(s, "tAIB", 4) && id == 0x03e8)
       tAIB = 1;
     if (!strncmp(s, "tAIB", 4) && id == 0x03e9)
       tAIBsmall = 1;
   }
  }

  if (!tAIB && !islib)
    pdb_addres(pdb, tAIB03e8, sizeof(tAIB03e8), "tAIB", 0x03e8);

  if (!tAIBsmall && !islib)
    pdb_addres(pdb, tAIB03e9, sizeof(tAIB03e9), "tAIB", 0x03e9);

  if (pdb_close(pdb) != 0)
    fatal("Can not create PRC");
 }
 lua_close(L);
 return 0;
}
