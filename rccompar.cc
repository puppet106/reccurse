// reccurse script scriptcommand parser
// included libraries
// C
#include <unistd.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>
// C++ 
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstdlib>
// stl
#include <vector>
#include <string>
#include <cstring>
#include <array>
#include <algorithm>

using namespace std;

// definitions
// numericals
const int MAXSTRING=80; // characters in a regular string
const int MAXTITLE=20; // characters in a a title string
const int MAXPAGES=25; // pages limit
const int MAXSUFFIXCHARS=3; // max string for number suffixes
const int MAXRELATIONSHIPS=25; // will fit into above 1kb, same as MAXPAGES
const int SPACE=32;
enum { HORIZONTALLY=1, VERTICALLY };
enum { NOBUTTON=0, TICKBOX, BUTTONBOX, BUTTONSCREEN, BUTTONCOMMAND, AUTOMATICSCRIPT };
enum { NUMERICAL=0, CALENDAR, STRING, MIXEDTYPE, VARIABLE, PROGRAM, CLOCK };
enum { NORMAL=0, STANDOUT, UNDERLINE, REVERSE, BLINK, DIM, BOLD, PROTECT, INVISIBLE };
enum { TOLEFT=1, CENTER, TORIGHT };
enum { SAME=0, LOWER, UPPER };
enum { NOCOMMAND=0, NONEXECUTABLE, SCRIPTCOMMAND, SKIPCOMMAND }; // return results
const char *noparametercommands[]= { "push", "copy", "paste", "end", "compare", "greater", "smaller", "clear", "autosave", "quit", "stop", "pass", "flash", "id", "pause", "save", "load", "activate", "highlight" };
enum { PUSHSPACE=0, COPYTEXT, PASTETEXT, ENDLOOP, COMPAREWITHCLIPBOARD, COMPAREWITHCLIPBOARDGREATER, COMPAREWITHCLIPBOARDSMALLER, CLEARVARIABLES, AUTOSAVERECCURSE,  QUITPROGRAM, STOPSCRIPT, PASS, FLASH, SHOWID, PAUSE, SAVEDATABASE, LOADDATABASE, ACTIVATEDEACTIVATEFIELD, TOGGLEHIGHLIGHTFIELD };
int noparametercommandskeys=19;
const char *parametercommands[]= { "file", "record", "field", "enter", "append", "variable", "variabletoclipboard", "delete", "loop", "wait", "goto", "page", "textattributes", "titleattributes", "textcolor", "titlecolor", "boxcolor", "sleep", "menu", "key", "keys", "push", "move", "waittime", "menubar" };
enum { FILEOPEN=0, GOTORECORD, GOTOFIELD, ENTERTEXT, APPENDTEXT, VARIABLESET, VARIABLETOCLIPBOARD, CLEARVARIABLE, LOOPFOR, WAITSECS, GOTOLABEL, GOTOPAGE, SETTEXTATTRIBUTES, SETTITLEATTRIBUTES, SETTEXTCOLOR, SETTITLECOLOR, SETBOXCOLOR, SETSLEEPTIME, CHANGEMENU, PUSHKEY, PUSHKEYS, PUSHSPACEFIELD, MOVEFIELD, WAITTIME, TOGGLEBAR };
int parametercommandskeys=25;
const char *systemvariables[] = { "version", "records", "record", "fields", "field", "page" };
enum { VERSION = 0, RECORDS, RECORD, FIELDS, FIELD, PAGE };
int systemvariableskeys=6;
enum { SEGMENTATIONFAULT = -4, FLOATINGPOINTEXCEPTION = -3, NOACTIVEFIELDS, FILEERROR, NORMALEXIT = 0 };
enum { READ = 0, WRITE, CREATEDB, RECREATE, RECREATERC };
enum { OFF = 0, ON };
enum { MAIN=0, OPTIONS, EDIT, EXTRA, EDITOR, CALCULATOR, EDITEXTRA, EXTERNALDB };
enum { TEXT, TITLE, BOX };
enum { RESTORE = 0, BACKUP };

const int SCRIPTSLEEPTIME=250;
int scriptrunning=0, scriptsleeptime=SCRIPTSLEEPTIME;
char scriptfile[MAXSTRING];

extern int fieldsperrecord;
extern int changedrecord, editoroption;
extern int currentrecord;
extern int recordsnumber;
extern int currentfield;
extern int alteredparameters;
extern int separatort, separatord, suffixposition;
extern int autosave;
extern char clipboard[MAXSTRING];
extern const char *EMPTYSTRING;
extern int currentpage;
extern int pagesnumber;
extern int currentmenu;
extern int alteredparameterstarget;
extern int runscript;
extern double version;
extern char pages[MAXPAGES][MAXSTRING];

struct Points {
 int x;
int y; } ;

class Field {
 public:
  // from .rc file
  int id;
  char title[MAXTITLE];
  int title_position;
  char title_attributes[9];
  int title_color;
  struct Points pt;
  struct Points size;
  char attributes[9];
  int color;
  int box;
  int box_color;
  int type; // 0 number, 1 date&time, 2 string, 3 mixed type etc
  int decimals;
  char suffix[MAXSUFFIXCHARS];
  int formula;
  int fieldlist;
  int editable;
  int active;
  char automatic_value[MAXSTRING];
  int buttonbox; // 0 none, 1 tickbox, 2 button, 3 button screen, 4 command, automatic script
  // constructors, destructor
  Field(int i1, char s1[MAXSTRING], int i2, char s2[9], int i3, int i4, int i5, int i6, int i7, char s3[9], int i8, int i9, int i10, int i11, char s4[MAXSUFFIXCHARS], int i12, int i13, int i14, int i15, int i16, char s5[MAXSTRING]) { id=i1; strcpy(title,s1); title_position=i2; strcpy(title_attributes,s2); title_color=i3; pt.x=i4; pt.y=i5; size.x=i6; size.y=i7; strcpy(attributes, s3); color=i8; box=i9; box_color=i10; type=i11; strcpy(suffix, s4);  decimals=i12, formula=i13; fieldlist=i14; editable=i15; active=i16;  strcpy(automatic_value, s5); buttonbox=NOBUTTON; } ;
  Field() { } ;
~Field() { } ; } ;

class Annotated_Field {
  // to be annotated in each record
  public:
   int id; // same as Field
   double number;
   char text[MAXSTRING];
   char formula[MAXSTRING];
   // constructors, destructor
   Annotated_Field(int i1, double f1, char s1[MAXSTRING], char s2[MAXSTRING]) { id=i1; number=f1; strcpy(text, s1); strcpy(formula, s2); } ;
   Annotated_Field(int i1, double f1, const char s1[MAXSTRING], const char s2[MAXSTRING]) { id=i1; number=f1; strcpy(text, s1); strcpy(formula, s2); } ;
   Annotated_Field(int i1, double f1, char s1[MAXSTRING], const char s2[MAXSTRING]) { id=i1; number=f1; strcpy(text, s1); strcpy(formula, s2); } ;
   Annotated_Field() { } ;
~Annotated_Field() { } ; } ;

class ScriptLine {
 public:
  char textLine[MAXSTRING];
  ScriptLine(char *s) { strcpy(textLine, s); };
  ScriptLine() { };
~ScriptLine() { };
};

extern struct Points pt;
vector<ScriptLine> scriptlines, bscriptlines;
extern vector<Field> record, dummyrecord, externalrecord[MAXRELATIONSHIPS];
extern vector<Annotated_Field> records, dummyrecords, externalrecords[MAXRELATIONSHIPS];
extern vector<int> keyonnextloop;

int runline=0, skiphighlight=OFF;

// function declarations
int commandparser(char scriptcommand[MAXSTRING]);
int labelposition(char *label);
int islinelabel(char *scriptcommand);
int setvariablefromfield(char *parameter, int field_id);
void stopscript();
int isvariable(char *parameter);
extern int scantextforcommand(char *text, char *command, char separator='@');
extern void pushspaceonfield(int field_id=-1);
extern void copytoclipboard();
extern void pastefromclipboard();
extern void toggleautosave();
extern int Add_Field(int type=NUMERICAL, char *name=NULL, char *textvalue=NULL);
extern void Delete_Field(int field_id);
extern int End_Program(int code=NORMALEXIT);
extern int tryfile(char *file);
extern long int fieldposition(int record_id, int field_id);
extern int Read_Write_Field(Annotated_Field &tfield, long int field_position, int mode=0);
extern int Pages_Selector(int pagetochange=-1);
extern int decimatestringtokey(char *text);
extern int breaktexttokeys(char *text);
extern void Flash_Field(int field_id, int sleeptime=SCRIPTSLEEPTIME);
extern int isrecordproperlydictated(Field &tfield);
extern int Show_Field_ID(Annotated_Field *tfield);
extern char* stringtolower(char *text);
extern int Read_Write_db_File(int mode=0);
extern void Load_Database(int pagenumber);
extern void togglemenubar(int pos=-1);

// parse by line
int commandparser(char scriptcommand[MAXSTRING])
{
  int i, i1, noparameters, commandtorun, thisfield, FAIL=0;
  double d;
  static int fileopen=0;;
  char tline[MAXSTRING], parameter[MAXSTRING];
  int returnvalue=SCRIPTCOMMAND;
  
   if (fileopen) {
    ifstream infile(scriptfile);
    scriptlines.clear();
    while (infile) {
     infile.getline(tline, MAXSTRING);
     if (!strlen(tline))
      continue;
     ScriptLine tscriptline(tline);
    scriptlines.push_back(tscriptline); }
    bscriptlines=scriptlines;
    infile.close();
    fileopen=0;
    scriptrunning=1;
   }
   
   // stop script procedures
   if ( (runline == (int)scriptlines.size() && scriptrunning) || !strcmp(scriptcommand, noparametercommands[STOPSCRIPT]) ) {
    stopscript();
    return NOCOMMAND;
   }
   
   if ( scriptrunning )
    strcpy(scriptcommand, scriptlines[runline++].textLine);
   thisfield=(currentrecord*fieldsperrecord)+currentfield; // commands only work on currentfield
   
   // split line to scriptcommand and parameter
   noparameters=scantextforcommand(scriptcommand, parameter, SPACE);
   stringtolower(parameter);
   
   // handle labels
   if ( islinelabel(scriptcommand) ) {
    return SCRIPTCOMMAND;
   }
   
   // variables replacement
   if (strcmp(scriptcommand, parametercommands[VARIABLESET]) && strcmp(scriptcommand, parametercommands[CLEARVARIABLE])) {
    for (i=0;i<(int)record.size();i++) {
     if (record[i].type==VARIABLE) {
      if (!strcmp(record[i].title, parameter) && strcmp(scriptcommand, parametercommands[VARIABLESET])) {
       strcpy(parameter, record[i].automatic_value);
      }
     }
    }
   }
   
   // system variables
   if ( noparameters ) {
    for (i=0;i<systemvariableskeys;i++)
     if ( !strcmp(parameter, systemvariables[i]) )
      break;
    if ( i < systemvariableskeys ) {
     switch ( i ) {
      case VERSION:
       sprintf(tline, "%f", version);
      break;
      case RECORDS:
       sprintf(tline, "%d", recordsnumber);
      break;
      case RECORD:
       sprintf(tline, "%d", currentrecord);
      break;
      case FIELDS:
       sprintf(tline, "%d", fieldsperrecord);
      break;      
      case FIELD:
       sprintf(tline, "%d", currentfield);
      break;
      case PAGE:
       sprintf(tline, "%s", pages[currentpage]);
      break;
     }
     strcpy(parameter, tline);
    }
   }
   
   if ( !noparameters ) {
     
    for (commandtorun=0;commandtorun<noparametercommandskeys;commandtorun++)
     if (!strcmp(noparametercommands[commandtorun], scriptcommand))
      break;
    switch (commandtorun) {
     case PUSHSPACE:
      pushspaceonfield();
     break;
     case COPYTEXT:
      copytoclipboard();
     break;
     case PASTETEXT:
      pastefromclipboard();
     break;
     case ENDLOOP:
      if (!scriptrunning)
       return NOCOMMAND;
      i1=runline-2; // endloop line-1
      strcpy(scriptcommand, noparametercommands[PASS]);
      while ( i1 > 0 && (strcmp(scriptcommand, parametercommands[LOOPFOR]))) {
       strcpy(scriptcommand, scriptlines[i1--].textLine);
       runscript=scantextforcommand(scriptcommand, parameter, SPACE);
      }
      if ( i1 == 0 && (strcmp(scriptcommand, parametercommands[LOOPFOR])) )
       stopscript();
      runline=i1+1;
      returnvalue=SKIPCOMMAND;
     break;
     case COMPAREWITHCLIPBOARD:
      if (!scriptrunning)
       return NONEXECUTABLE;
      d=atof(clipboard);
      if (record[currentfield].type==NUMERICAL && records[thisfield].number!=d) {
       ++runline; // jump next line
      break; }
      if (record[currentfield].type==MIXEDTYPE && d && d!=records[thisfield].number) {
       ++runline;
      break; }
      if (strcmp(clipboard, records[thisfield].text))
       ++runline;
     break;
     case COMPAREWITHCLIPBOARDGREATER:
      if (!scriptrunning)
       return NONEXECUTABLE;
      d=atof(clipboard);
      if ((record[currentfield].type!=NUMERICAL && record[currentfield].type!=MIXEDTYPE) || !d) {
       ++runline;
      break; }
      if (records[thisfield].number<=d)
        ++runline;
     break;
     case COMPAREWITHCLIPBOARDSMALLER:
      if (!scriptrunning)
       return NONEXECUTABLE;
      d=atof(clipboard);
      if ((record[currentfield].type!=NUMERICAL && record[currentfield].type!=MIXEDTYPE) || !d) {
       ++runline;
      break; }
       if (records[thisfield].number>=d)
        ++runline;
     break;
     case AUTOSAVERECCURSE:
      toggleautosave();
     break;
     case CLEARVARIABLES:
      for (i1=0;i1<(int)record.size();i1++)
       if (record[i1].type==VARIABLE)
        Delete_Field(i1);
     break;
     case QUITPROGRAM:
      End_Program(NORMALEXIT);
     break;
     case STOPSCRIPT:
      // covered above
     break;
     case PASS:
      // do nothing
      returnvalue=SKIPCOMMAND;
     break;
     case FLASH:
      Flash_Field(currentfield);
     break;
     case SHOWID:
      editoroption=-2; // let id be seen for 1 sec
      Show_Field_ID(&records[thisfield]);
      editoroption=0;
     break;
     case PAUSE:
      getch();
     break;
     case SAVEDATABASE:
      alteredparameters=0;
      Read_Write_db_File(RECREATE);
      Read_Write_db_File(WRITE); 
     break;
     case LOADDATABASE:
      Load_Database(currentpage);
     break;
     case ACTIVATEDEACTIVATEFIELD:
      record[currentfield].active = (record[currentfield].active == ON) ? OFF : ON;
     break;
     case TOGGLEHIGHLIGHTFIELD:
      skiphighlight = (skiphighlight == ON) ? OFF : ON;
     break;
     default:
      if (scriptcommand[strlen(scriptcommand)-1]==':') // label
       break;
      if (scriptrunning)
       return NONEXECUTABLE;
      return NOCOMMAND;
    break; }
    
   }
   else { // parameter scriptcommand
     
    for (commandtorun=0;commandtorun<parametercommandskeys;commandtorun++)
     if (!strcmp(parametercommands[commandtorun], scriptcommand))
      break;
    switch (commandtorun) {
     case FILEOPEN:
      if (!tryfile(parameter))
       return NONEXECUTABLE;
      stopscript();
      fileopen=1;
      strcpy(scriptfile, parameter);
     break;
     case GOTORECORD:
      i1=atoi(parameter)-1;
      if (!strcmp(parameter, "+"))
       i1=currentrecord+1;
      if (!strcmp(parameter, "-"))
       i1=currentrecord-1;
      if (i1<0 || i1>recordsnumber)
       returnvalue=FAIL;
      else
       currentrecord=i1;
     break;
     case GOTOFIELD:
      i1=atoi(parameter)-1;
      if (!strcmp(parameter, "+"))
       i1=currentfield+1;
      if (!strcmp(parameter, "-"))
       i1=currentfield-1;
      if (i1<0 || i1>fieldsperrecord-1)
       returnvalue=FAIL;
      else
      currentfield=i1;
     break;
     case ENTERTEXT:
      if (strcmp(record[currentfield].automatic_value, EMPTYSTRING) || !record[currentfield].editable)
       returnvalue=FAIL;
      else
       strcpy(records[thisfield].text, parameter);
      if (autosave)
       Read_Write_Field(records[thisfield], fieldposition(currentrecord, currentfield), 1);
     break;
     case APPENDTEXT:
      if (strcmp(record[currentfield].automatic_value, EMPTYSTRING) || !record[currentfield].editable)
       returnvalue=FAIL;
      else
       strcat(records[thisfield].text, parameter);
      if (autosave)
       Read_Write_Field(records[thisfield], fieldposition(currentrecord, currentfield), 1);
     break;
     case VARIABLESET:   
      i1=setvariablefromfield(parameter, currentfield);
      // place values in clipboard
      strcpy(clipboard, record[i1].automatic_value);
     break;
     case VARIABLETOCLIPBOARD:
      for (i1=0;i1<(int) record.size();i1++)
       if (!strcmp(record[i1].title, parameter))
        break;
      if (i1==(int) record.size())
       returnvalue=FAIL;
      else
      strcpy(clipboard, record[i1].automatic_value);
     break;
     case CLEARVARIABLE:
      if ((i1=isvariable(parameter))==0)
       returnvalue=FAIL;
      else
      Delete_Field(i1-1);
     break;
     case LOOPFOR:
      if ( !scriptrunning )
       return NOCOMMAND;
      i1=atoi(parameter);
      if ( i1 < 1 ) { // loop has reached 0
       strcpy(scriptlines[runline-1].textLine, noparametercommands[PASS]);
       while ( (strcmp(scriptlines[runline].textLine, noparametercommands[ENDLOOP])) && runline < (int)scriptlines.size() )
        ++runline;
       if ( runline < (int)scriptlines.size() )
        strcpy(scriptlines[runline].textLine, noparametercommands[PASS]);
       ++runline;
      }
      else
       sprintf(scriptlines[runline-1].textLine, "%s %d", parametercommands[LOOPFOR], i1-1);
      returnvalue=SKIPCOMMAND;
     break;
     case WAITSECS:
      i1=atoi(parameter);
      if (i1>0)
       returnvalue=FAIL;
      else
       sleep(atoi(parameter));
     break;
     case GOTOLABEL:
      if ((i1=labelposition(parameter))==0)
       returnvalue=FAIL;
      else
       scriptlines=bscriptlines; // restore lines for loop adjustments
      runline=i1;
     break;
     case GOTOPAGE:
      i1=atoi(parameter)-1;
      if (!strcmp(parameter, "+"))
       i1=currentpage+1;
      if (!strcmp(parameter, "-"))
       i1=currentpage-1;
      if (i1<0 || i1>pagesnumber-1)
       return NOCOMMAND; // will fail in script as well
      Pages_Selector(i1);
     break;
     case SETTEXTATTRIBUTES:
      if (strlen(parameter)!=9) {
       returnvalue=FAIL;
       break;
      }
      for (i1=0;i1<9;i1++) {
       if (parameter[i1]!='0' && parameter[i1]!='1') {
        break;
       }
      }
      if (i1<9)
       returnvalue=FAIL;
      else {
       strcpy(record[currentfield].attributes, parameter);
       alteredparameters=1;
      }
     break;
     case SETTITLEATTRIBUTES:
      if (strlen(parameter)!=9) {
       returnvalue=FAIL;
       break;
      }
      for (i1=0;i1<9;i1++) {
       if (parameter[i1]!='0' && parameter[i1]!='1') {
        break;
       }
      }
      if (i1<9)
       returnvalue=FAIL;
      else {
       strcpy(record[currentfield].title_attributes, parameter);
       alteredparameters=1;
      }
     break;
     case SETTEXTCOLOR:
      i1=atoi(parameter);
      if (i1<1 || i1>58)
       returnvalue=FAIL;
      else {
       record[currentfield].color=i1;
       alteredparameters=1;
      }
     break;
     case SETTITLECOLOR:
      i1=atoi(parameter);
      if (i1<1 || i1>58)
       returnvalue=FAIL;
      else {
       record[currentfield].title_color=i1;
       alteredparameters=1;
      }
     break;
     case SETBOXCOLOR:
      i1=atoi(parameter);
      if (i1<1 || i1>58)
       returnvalue=FAIL;
      else {
       record[currentfield].box_color=i1;
       alteredparameters=1;
      }
     break;
     case SETSLEEPTIME:
      i1=atoi(parameter);
      if (!i1)
       returnvalue=FAIL;
      else
       scriptsleeptime=i1;
     break;
     case CHANGEMENU:
      i1=atoi(parameter);
      if (i1<0 || i1>5 || i1==4)
       returnvalue=FAIL;
      else
       currentmenu=i1;
     break;
     case PUSHKEY:
      if ( (i1=decimatestringtokey(parameter))==0 )
       returnvalue=FAIL;
      else
       keyonnextloop.push_back(i1);
     break;
     case PUSHKEYS:
      breaktexttokeys(parameter);
     break;
     case PUSHSPACEFIELD:
      i1=atoi(parameter);
      if ( i1 )
       pushspaceonfield(atoi(parameter)-1);
     break;
     case MOVEFIELD: {
      const char *MOVEPARAMETERS[] = { "up", "down", "left", "right", "upleft", "upright", "downleft", "downright" };
      enum { FU, FD, FL, FR, FUL, FUR, FDL, FDR };
      Field tfield=record[currentfield];
      for (i=FU;i<=FDR;i++)
       if ( !strcmp(MOVEPARAMETERS[i], parameter) )
        break;
      if ( i == FDR+1 ) {
       returnvalue=FAIL;
       break;
      }
      switch ( i ) {
       case FU:
        tfield.pt.y--;
       break;
       case FD:
        tfield.pt.y++;
       break;
       case FL:
        tfield.pt.x--;
       break;
       case FR:
        tfield.pt.x++;
       break;
       case FUL:
        tfield.pt.x--;
        tfield.pt.y--;
       break;
       case FUR:
        tfield.pt.x++;
        tfield.pt.y--;
       break;
       case FDL:
        tfield.pt.x--;
        tfield.pt.y++;
       break;
       case FDR:
        tfield.pt.x++;
        tfield.pt.y++;
       break;
      }
      if ( isrecordproperlydictated(tfield) ) {
       record[currentfield]=tfield;
       alteredparameters=1; 
      }
      else
       returnvalue=FAIL;
     }
     break;
     case WAITTIME:
      i1=atoi(parameter);
      if ( i1 > 0 )
       scriptsleeptime=i1;
     break;
     case TOGGLEBAR:
      i1=atoi(parameter);
      if ( -1<i1<4 )
       togglemenubar(i1);
     break;
     default:
      if (scriptrunning)
       return NONEXECUTABLE;
      return NOCOMMAND;
     break;
    }
    
   }
   
   strcpy(scriptcommand, " ");

   if ( returnvalue == FAIL )
    returnvalue=(scriptrunning) ? NONEXECUTABLE: NOCOMMAND;
   
 return returnvalue;
}

// locate label
int labelposition(char *label)
{
  int i, i1;
  char tlabel[MAXSTRING];
  strcpy(tlabel, label);
  strcat(tlabel, ":");
  
   // find label
   for (i=0;i<(int) scriptlines.size();i++)
    if (!strcmp(tlabel, scriptlines[i].textLine))
     break;
   // duplicate ?
   for (i1=i+1;i1<(int) scriptlines.size();i1++)
    if (!strcmp(tlabel, scriptlines[i1].textLine))
    break;
    
   if (i==(int) scriptlines.size() || i1<(int) scriptlines.size())
    return 0;
  
 return i;
}

// is scriptcommand a label
int islinelabel(char *scriptcommand)
{
  if (scriptcommand[strlen(scriptcommand)-1]==':')
   return 1;
  
 return 0;
}

// set variable from field
int setvariablefromfield(char *parameter, int field_id)
{
  int i;
  int thisfield=(currentrecord*fieldsperrecord)+field_id;
  
   i=isvariable(parameter);
   
   if (i) // variable exists
    strcpy(record[i-1].automatic_value, records[thisfield].text);
   else
    Add_Field(VARIABLE, parameter, records[thisfield].text);
   
 return i;
}

// stop script
void stopscript()
{
    scriptlines.clear();
    runline=scriptrunning=skiphighlight=0;
    scriptsleeptime=SCRIPTSLEEPTIME;
}

// return variable field id
int isvariable(char *parameter)
{
  int i;
  
   for (i=0;i<(int) record.size();i++)
    if (record[i].type==VARIABLE)
     if (!strcmp(record[i].title, parameter))
      break;
    
 return (i==(int) record.size()) ? 0 : i+1;
}

