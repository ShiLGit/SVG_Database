#include "SVGParser.h"
#include "LinkedListAPI.h"
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

//"public" functions
SVGimage* createSVGimage(char*);
char* SVGimageToString(SVGimage*);
void deleteSVGimag(SVGimage*);

List* getRects(SVGimage*);
List* getCircles(SVGimage*);
List* getGroups(SVGimage*);
List* getPaths(SVGimage*);

int numRectsWithArea(SVGimage*, float);
int numCirclesWithArea(SVGimage*, float);
int numPathsWithdata(SVGimage*, char*);
int numGroupsWithLen(SVGimage*, int);

int numAttr(SVGimage*);

//helper functions
void deleteAttribute(void*);
char* attributeToString(void*);
int compareAttribtues(const void*, const void*);

void deleteGroup(void*);
char* groupToString( void*);
int compareGroups(const void*, const void*);

void deleteRectangle(void*);
char* rectangleToString(void*);
int compareRectangles(const void*, const void*);

void deleteCircle(void*);
char* circleToString(void*);
int compareCircles(const void*, const void*);

void deletePath(void*);
char* pathToString(void*);
int comparePaths(const void*, const void*);

//custom helpers
Group* allocGroup(xmlNode*);
Circle* parseCircle(xmlNode*);
Rectangle* parseRectangle(xmlNode*);
Path* parsePath(xmlNode*);
void printGroupStructure(List*, int);
int numRectsWithArea(SVGimage*, float);
int listLength(List*);
int groupLength(Group*);
void safeFree(void*);
void smartConcat(char**, char*);
void parseUnit(char*, char*);
void initLists(SVGimage*);
char** valuesJSON(const char *svgString);
bool hasFileExt(char*);
xmlDoc* parseTree(SVGimage*);
void addChildRects(xmlNode*, List*);
void addChildCircs(xmlNode*, List*);
void addChildPaths(xmlNode*, List*);
void addChildGroups(xmlNode*, List*);
void addOtherAttr(xmlNode*, List*);

char* helloWorld(){
  char* x = malloc(30);
  strcpy(x, "Hello world");
  return NULL;
}

//Given filename (INCLUDING DIRECTORY FROM ROOT), return JSON format; return NULL if invalid file
char* fileNameToJSON(char* fileName){
  SVGimage* svg = createValidSVGimage(fileName, "parser/svg.xsd");
  if(!svg){
    return NULL;
  }
  char* json = SVGtoJSON(svg);
  free(svg);
  return json;
}
//Given filename (INCLUDING DIRECTORY FROM ROOT), return JSON format; return NULL if invalid file

char* fileNameToDetailedJSON(char* fileName){
  char buffer[2000];
  char* toReturn, *rList, *cList, *pList, *grList, *aList;
  SVGimage* svg = createValidSVGimage(fileName, "parser/svg.xsd");
  if(!svg){
    return NULL;
  }
  rList = rectListToJSON(svg->rectangles);
  cList = circListToJSON(svg->circles);
  pList = pathListToJSON(svg->paths);
  grList = groupListToJSON(svg->groups);
  aList = attrListToJSON(svg->otherAttributes);

  sprintf(buffer, "{\"title\":\"%s\",\"desc\":\"%s\",\"rects\":%s,\"circs\":%s,\"paths\":%s,\"groups\":%s,\"attr\":%s}",
                   svg->title, svg->description, rList, cList, pList, grList, aList);
  toReturn = malloc(strlen(buffer) + 1);
  strcpy(toReturn, buffer);
  
  free(svg);
  free(rList);
  free(cList);
  free(pList);
  free(grList);
  free(aList);
  return toReturn;
}
/*******************************MODULE 2.1****************************/
SVGimage* createValidSVGimage(char* fileName, char* schemaFile){
  
  if(fileName == NULL || schemaFile == NULL){
    return NULL;
  }

  //xsd stuff that TOTALLY WASNT COPY PASTED!!
  xmlDocPtr xmlFile;
  xmlSchemaPtr schema = NULL;
  xmlSchemaParserCtxtPtr ctxt;

  //mek sure context valid, watever that is 
  xmlLineNumbersDefault(1);
  ctxt = xmlSchemaNewParserCtxt(schemaFile);
  if(ctxt == NULL){
    printf("\nError: NULL context!");
    xmlCleanupParser();
    return NULL;
  }

  //skeema stuff
  xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
  schema = xmlSchemaParse(ctxt);
  
  if(schema == NULL){
    printf("\nwtf fam");
    xmlSchemaFreeParserCtxt(ctxt);
    xmlSchemaCleanupTypes();
    xmlCleanupParser();

    return NULL;
  }
  xmlSchemaFreeParserCtxt(ctxt);

  //is doc valid?
  xmlFile = xmlReadFile(fileName, NULL, 0);
  if(xmlFile == NULL)
  {
    xmlSchemaFree(schema);
    xmlSchemaCleanupTypes();
    xmlCleanupParser();

    fprintf(stderr, "From createValidSVGimage: Could not parse %s\n", fileName);
    return NULL;
  }
  //namespace stuff...?
  xmlNodePtr root = xmlDocGetRootElement(xmlFile);
  xmlSetNs(root, root->ns);

  xmlSchemaValidCtxtPtr vctxt;
  int ret;

  vctxt = xmlSchemaNewValidCtxt(schema);
  if(vctxt == NULL){
    xmlSchemaFree(schema);
    xmlFreeDoc(xmlFile);
    xmlSchemaCleanupTypes();
    xmlCleanupParser();
    return NULL;
  }
  xmlSchemaSetValidErrors(vctxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
  ret = xmlSchemaValidateDoc(vctxt, xmlFile);
    
  if (ret == 0)
  {
    printf("From createValidSVGimage: %s validated.\n", fileName);

    xmlSchemaFree(schema);
    xmlSchemaFreeValidCtxt(vctxt);
    xmlFreeDoc(xmlFile);

    xmlSchemaCleanupTypes();
    xmlCleanupParser();
    return createSVGimage(fileName);
  }
  else if (ret > 0)
  {
    printf("From createValidSVGimage: %s fails to validate\n", fileName);
  }
  else
  {
    printf("From createValidSVGimage: %s validation generated an internal error\n", fileName);
  }

  xmlSchemaFree(schema);
  xmlSchemaFreeValidCtxt(vctxt);
  xmlFreeDoc(xmlFile);

  xmlSchemaCleanupTypes();
  xmlCleanupParser();
  xmlMemoryDump();
 
  return NULL;
}
//cpppppppaaaaaaaaaaaaaaaaaaaaaaaaaaaaaastaaaaaaa
bool vsvg_checkGroup(Group*);
bool vsvg_checkAttr(Attribute* attr){
  if(attr == NULL){
    printf("\nEVSVG: null attribute");
    return false;
  }
  if(attr->name == NULL || attr->value == NULL){
    printf("\nEVSVG: Nnull attr name or val");
    return false;
  }
  return true;
}
bool vsvg_checkRect(Rectangle* rect){
  if(rect == NULL){
    printf("\nError vsvg: null rectangle.");
    return false;
  }
  if(rect->width < 0 || rect->height < 0){
    printf("\nError vsvg: negative w/h rect.");
    return false;
  }

  //attr?
  if(rect->otherAttributes == NULL){
    printf("\nEVSVG: Null otherattributes.");
    return false;
  }
  ListIterator iter = createIterator(rect->otherAttributes);
  void* elem;
  while ((elem = nextElement(&iter)) != NULL){
    Attribute* tmp = (Attribute*)elem;
    if(vsvg_checkAttr(tmp) == false){
      printf("\nvsvgr: invalid attr.");
      return false;
    }
  }
  return true;
}
bool vsvg_checkCirc(Circle* circ){
  if(circ == NULL){
    printf("\nError vsvg: null circle");
    return false;
  }
  if(circ->r < 0){
    printf("\nError vsvg: negative r");
    return false;
  }

  if(circ->otherAttributes == NULL)
    return false;

  ListIterator iter = createIterator(circ->otherAttributes);
  void* elem;
  while ((elem = nextElement(&iter)) != NULL){
    Attribute* tmp = (Attribute*)elem;
    if(vsvg_checkAttr(tmp) == false){
      printf("\nvsvgc: invalid attr");
      return false;
    }
  }

  return true;
}
bool vsvg_checkPath(Path* p){
  if(p == NULL){
    return false; 
    printf("\nvsgp: null");
  }
  if(!p->data){
    printf("\nvsvgp: data is null.");
    return false;
  }

  if(p->otherAttributes == NULL){
    printf("\nvsgp: other attr null");
    return false;
  }

  ListIterator iter = createIterator(p->otherAttributes);
  void* elem;
  while ((elem = nextElement(&iter)) != NULL){
    Attribute* tmp = (Attribute*)elem;
    if(vsvg_checkAttr(tmp) == false){
      printf("\nvsgp: attr invalid");
      return false;
    }
  }
  return true;
}
bool vsvg_checkAllGroups(List* list){
  ListIterator iter = createIterator(list);
  void* elem;

  while ((elem = nextElement(&iter)) != NULL){
    Group* tmp = (Group*)elem;
    if(vsvg_checkGroup(tmp) == false)
      return false;

    if(!vsvg_checkAllGroups(tmp->groups)){
      return false;
    }
  }
  return true;
}
bool vsvg_checkGroup(Group* g){
  if(g == NULL)
    return false;

  if(g->rectangles == NULL || g->circles == NULL || g->paths == NULL || g->groups == NULL || g->otherAttributes == NULL){
    return false;
  }

  ListIterator iter = createIterator(g->rectangles);
  void* elem;

  //check rectangles
  while ((elem = nextElement(&iter)) != NULL){
    Rectangle* tmp = (Rectangle*)elem;
    if(vsvg_checkRect(tmp) == false)
      return false;
  }
  //check circs
  iter = createIterator(g->circles);
  while ((elem = nextElement(&iter)) != NULL){
    Circle* tmp = (Circle*)elem;
    if(vsvg_checkCirc(tmp) == false)
      return false;
  }
  //check paths
  iter = createIterator(g->paths);
  while ((elem = nextElement(&iter)) != NULL){
    Path* tmp = (Path*)elem;
    if(vsvg_checkPath(tmp) == false)
      return false;
  }
  return true;
}
//helper fx: is img compliant with constraings in svgparser.h?
//PRE: IMG IS NOT NULL!
bool vsvg_checkConstraints(SVGimage *img){
  //are lists empty? 
  if(img->rectangles == NULL || img->circles == NULL || img->paths == NULL|| img->groups == NULL|| img->otherAttributes==NULL){
    printf("\nError valid8svgi: null list\n");
    return false;
  }

  ListIterator iter = createIterator(img->rectangles);
  void* elem;

  //check rectangles
  while ((elem = nextElement(&iter)) != NULL){
    Rectangle* tmp = (Rectangle*)elem;
    if(vsvg_checkRect(tmp) == false)
      return false;
  }
  //check circs
  iter = createIterator(img->circles);
  while ((elem = nextElement(&iter)) != NULL){
    Circle* tmp = (Circle*)elem;
    if(vsvg_checkCirc(tmp) == false)
      return false;
  }

  //check paths
  iter = createIterator(img->paths);
  while ((elem = nextElement(&iter)) != NULL){
    Path* tmp = (Path*)elem;
    if(vsvg_checkPath(tmp) == false)
      return false;
  }
  //check groups
  if(!vsvg_checkAllGroups(img->groups)){
    return false;
  }
  return true;
}
//////////////enddddddd paaaaaaaaastaaaaaa
bool validateSVGimage(SVGimage* image, char* schemaFile){
  if(image == NULL || schemaFile == NULL){
    printf("\nError validateSVGimage: null argument.");
    return false;
  }
  
  if(vsvg_checkConstraints(image) == false){
    printf("\nvsvg checkconstr failed!!");
    return false;

  }

  xmlSchemaPtr schema;
  //check ctxt, watever that is 
  xmlSchemaParserCtxtPtr ctxt; 
  xmlLineNumbersDefault(1);
  ctxt = xmlSchemaNewParserCtxt(schemaFile);
  if(ctxt == NULL){
    return false; 
  }

  //schema stuff, watever that is 
  xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
  schema = xmlSchemaParse(ctxt);
  if(schema == NULL){
    xmlSchemaFreeParserCtxt(ctxt);
    return false; 
  }
  xmlSchemaFreeParserCtxt(ctxt);
  xmlDoc* doc = parseTree(image);

  xmlSchemaValidCtxtPtr vCtxt;
  int flag; 

  //lol ffs
  vCtxt = xmlSchemaNewValidCtxt(schema);
  if(vCtxt == NULL){
    xmlSchemaFree(schema);
    xmlFreeDoc(doc);
    xmlSchemaCleanupTypes();
    xmlCleanupParser();
    return false;
  }

  xmlSchemaSetValidErrors(vCtxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
  flag = xmlSchemaValidateDoc(vCtxt, doc);
  

  xmlSchemaFree(schema);
  xmlSchemaFreeValidCtxt(vCtxt);
  xmlFreeDoc(doc);

  xmlSchemaCleanupTypes();
  xmlCleanupParser();
  if(flag == 0){
    return true;
  }
  return false;
}

bool writeSVGimage(SVGimage* image, char* fileName){
  //check filename 
  if(fileName == NULL || strlen(fileName) < 1){
    printf("\nFrom writeSVGimage: invalid filename %s.", fileName);
    return false;
  }

  char* validFileName = malloc(sizeof(numCirclesWithArea)* (strlen(fileName) + 1));  
  strcpy(validFileName, fileName);
  if(!hasFileExt(fileName)){
    printf("\nNote: Invalid file name: concatenated .svg\n");
    smartConcat(&validFileName, ".svg");
  }
  
  xmlDoc* doc = parseTree(image);

  //save doc into file
  xmlSaveFormatFileEnc(validFileName, doc, "UTF-8", 1);
  
  //no leaki memori!
  free(validFileName);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  xmlMemoryDump();
  return true;
}
/*************************MODULE 2.2*************************************/
void setAttributeSVG(SVGimage*, Attribute*);
void setAttributeCirc(SVGimage*, int, Attribute*);
void setAttributeRect(SVGimage*, int, Attribute*);
void setAttributePath(SVGimage*, int, Attribute*);
void setAttributeGroup(SVGimage*, int, Attribute*);

char* getAttributeGroup(SVGimage*, int);
char* getAttributeRect(SVGimage*, int);
char* getAttributePath(SVGimage*, int);
char* getAttributeCirc(SVGimage*, int);

int sa_attrReplaced(List*, Attribute*);
int sa_preliminaryCheckAttr(Attribute*);
void sa_addNewAttr(List*, Attribute*);
void sa_freeAttr(Attribute*);

void setAttribute(SVGimage* image, elementType elemType, int elemIndex, Attribute* newAttribute){
  if(elemType == SVG_IMAGE){
    setAttributeSVG(image, newAttribute);
  }else if(elemType == CIRC){
    setAttributeCirc(image, elemIndex, newAttribute);
  }else if(elemType == RECT){
    setAttributeRect(image, elemIndex, newAttribute);
  }else if(elemType == PATH){
    setAttributePath(image, elemIndex, newAttribute);
  }else if(elemType == GROUP){
    setAttributeGroup(image, elemIndex, newAttribute);
  }
}
char* getAttribute(char* fileName, elementType elemType, int elemIndex){
  SVGimage *svg = createValidSVGimage(fileName, "parser/svg.xsd");
  char* toReturn;
  if(svg == NULL){
    return "{ERROR: Invalid SVG.}";
  }
  if(elemType == RECT){
    toReturn = getAttributeRect(svg, elemIndex);
  }else if(elemType == CIRC){
    toReturn = getAttributeCirc(svg, elemIndex);
  }else if (elemType == PATH){
    toReturn = getAttributePath(svg, elemIndex);
  }else if(elemType == GROUP){
    toReturn = getAttributeGroup(svg, elemIndex);
  }else{
    return NULL;
  }
  return toReturn;
}

///add copmpooooooooooooooonent 
void addComponent(SVGimage* image, elementType type, void* newElement){
  if(image == NULL || newElement == NULL){
    printf("\nError from addComponent: NULL argument...");
    return;
  }

  if(type == CIRC){
    if(image->circles == NULL){
      printf("\nErr from addComponent: NULL list..");
      return;
    }
    Circle* c = (Circle*)newElement;
    if(vsvg_checkCirc(c)){
      printf("\nValid Circle.");
      insertBack(image->circles, (void*)c);
      return;
    }
  }else if(type ==RECT){
    if(image->rectangles == NULL)
      return;
    Rectangle* r = (Rectangle*)newElement;
    if(vsvg_checkRect(r)){
      printf("\nValid Rect.");
      insertBack(image->rectangles, newElement);
      return;
    }
  }else if(type == PATH){
    if(image->paths == NULL)
      return;
    Path* p = (Path*)newElement;
    if(vsvg_checkPath(p)){
      printf("\nValid path.");
      insertBack(image->paths, newElement);
      return;
    }

  }
  printf("\nERROR: NO COMPONENT ADDED! Rect/Path/Circ/Group failed individual validation.");
}

/**************************helper fx MODULE 2.2**************************/
char* getAttributeGroup(SVGimage* img, int index){
  char* toReturn;
  ListIterator iter = createIterator(img->groups);
  
  void* elem; 
  int curIndex = 0;
  Group* dst = NULL;
  
  while ((elem = nextElement(&iter)) != NULL){
    if(curIndex == index){
      dst = (Group*)elem;
      break;
    }else{
       curIndex++;
    }
  }
  toReturn = attrListToJSON(dst->otherAttributes);
  return toReturn;
}
char* getAttributeRect(SVGimage* img, int index){
  char* toReturn;
  ListIterator iter = createIterator(img->rectangles);
  
  void* elem; 
  int curIndex = 0;
  Rectangle* dst = NULL;
  
  while ((elem = nextElement(&iter)) != NULL){
    if(curIndex == index){
      dst = (Rectangle*)elem;
      break;
    }else{
       curIndex++;
    }
  }
  toReturn = attrListToJSON(dst->otherAttributes);
  return toReturn;
}
char* getAttributeCirc(SVGimage* img, int index){
  char* toReturn;
  ListIterator iter = createIterator(img->circles);
  
  void* elem; 
  int curIndex = 0;
  Circle* dst = NULL;
  
  while ((elem = nextElement(&iter)) != NULL){
    if(curIndex == index){
      dst = (Circle*)elem;
      break;
    }else{
       curIndex++;
    }
  }
  toReturn = attrListToJSON(dst->otherAttributes);
  return toReturn;
}
char* getAttributePath(SVGimage* img, int index){
  char* toReturn;
  ListIterator iter = createIterator(img->paths);
  
  void* elem; 
  int curIndex = 0;
  Path* dst = NULL;
  
  while ((elem = nextElement(&iter)) != NULL){
    if(curIndex == index){
      dst = (Path*)elem;
      break;
    }else{
       curIndex++;
    }
  }
  toReturn = attrListToJSON(dst->otherAttributes);
  return toReturn;
}
//---------------------end of getattribtues
void setAttributeGroup(SVGimage* img, int index, Attribute *newAttribute){
  if(sa_preliminaryCheckAttr(newAttribute) == 0){
    return;
  }

  ListIterator iter = createIterator(img->groups);
  void* elem; 
  int curIndex = 0;
  Group* dst = NULL;
  int caseExed = 0;
  
  //find rect at given index 
  while ((elem = nextElement(&iter)) != NULL){
    if(curIndex == index){
      dst = (Group*)elem;
      break;
    }else{
       curIndex++;
    }
  }
  if(dst == NULL){
    printf("\nFrom setAttribute: Index %d out of bounds.", index);
    return;
  }

   //case 1: attr replaces member of otherAttributes list 
  if(caseExed == 0){
    if(sa_attrReplaced(dst->otherAttributes, newAttribute) == 1){
      caseExed = 1;
    }
  }

  //case 2: entirely new attribute in otherAttributes
  if(caseExed == 0){
     insertBack(dst->otherAttributes, newAttribute);
     caseExed = 3;
  }
  if(caseExed != 3)
    sa_freeAttr(newAttribute);
}

void setAttributePath(SVGimage* img, int index, Attribute* newAttribute){
  if(img == NULL || img->paths == NULL || sa_preliminaryCheckAttr(newAttribute) == 0){
    printf("\nsetattr Err: invalid attribute");
    return;
  }
  char* name = newAttribute->name;
  char* value = newAttribute->value;

  ListIterator iter = createIterator(img->paths);
  void* elem; 
  int curIndex = 0;
  Path* dst = NULL;
  int caseExed = 0;
  
  //find rect at given index 
  while ((elem = nextElement(&iter)) != NULL){
    if(curIndex == index){
      dst = (Path*)elem;
      break;
    }else{
       curIndex++;
    }
  }
  if(dst == NULL){
    printf("\nFrom setAttribute: Index %d out of bounds.", index);
    return;
  }

  //case 1: attribute is a field of the struct 
  if(strcmp(name, "d") == 0){
    if(dst->data)
      free(dst->data);
    
    dst->data = malloc(sizeof(char) * (strlen(value) + 1));
    strcpy(dst->data, value);
    caseExed = 1;
  }
  
  //case 2: attr replaces member of otherAttributes list 
  if(caseExed == 0){
    if(sa_attrReplaced(dst->otherAttributes, newAttribute) == 1){
      caseExed = 1;
    }
  }

  //case 3: entirely new attribute in otherAttributes
  if(caseExed == 0){
     insertBack(dst->otherAttributes, newAttribute);
     caseExed = 3;
  }
  if(caseExed != 3)
    sa_freeAttr(newAttribute);
}
void setAttributeRect(SVGimage* img, int index, Attribute* newAttribute){
  if(img == NULL || img->rectangles == NULL || sa_preliminaryCheckAttr(newAttribute) == 0){
    return;
  }

  char* name = newAttribute->name;
  char* value = newAttribute->value;
  
  ListIterator iter = createIterator(img->rectangles);
  void* elem; 
  int curIndex = 0;
  Rectangle* dst = NULL;
  int caseExed = 0;
  
  //find rect at given index 
  while ((elem = nextElement(&iter)) != NULL){
    if(curIndex == index){
      dst = (Rectangle*)elem;
      break;
    }else{
       curIndex++;
    }
  }

  if(dst == NULL){
    printf("\nFrom setAttribute: Index %d out of bounds.", index);
    return;
  }

  //case 1: attribute is a field of the struct 
  if(strcmp(name, "x") == 0){
    dst->x = strtof(value, NULL);
    parseUnit(value, dst->units);
    (dst->units)[49] = '\0';
    caseExed = 1;
  }else if(strcmp(name, "y") == 0){
    dst->y = strtof(value, NULL);
    parseUnit(value, dst->units);
    (dst->units)[49] = '\0';
    caseExed = 1;
  }else if(strcmp(name, "width") == 0){
    dst->width = strtof(value, NULL);
    parseUnit(value, dst->units);
    (dst->units)[49] = '\0';
    caseExed = 1;
  }else if(strcmp(name, "height") == 0){
    dst->height = strtof(value, NULL);
    parseUnit(value, dst->units);
    (dst->units)[49] = '\0';
    caseExed = 1;
  }else if(strcmp(name, "units") == 0){
    parseUnit(value, dst->units);
    (dst->units)[49] = '\0';
    caseExed = 1;
  }
  //case 2: attr replaces member of otherAttributes list 
  if(caseExed == 0){
    if(sa_attrReplaced(dst->otherAttributes, newAttribute) == 1){
      caseExed = 1;
    }
  }

  //case 3: entirely new attribute in otherAttributes
  if(caseExed == 0){
    insertBack(dst->otherAttributes, newAttribute);
    caseExed = 3;
  }
  
  if(caseExed != 3)
    sa_freeAttr(newAttribute);
}
void setAttributeCirc(SVGimage* img, int index, Attribute* newAttribute){
    if(img == NULL || img->circles == NULL || sa_preliminaryCheckAttr(newAttribute) == 0){
      return;
    }
    char* name = newAttribute->name;
    char* value = newAttribute->value;

    ListIterator iter = createIterator(img->circles);
    void* elem; 
    int curIndex = 0;
    Circle* dst = NULL;
    int caseExed = 0;
    //find circle at given index 
    while ((elem = nextElement(&iter)) != NULL){
      if(curIndex == index){
        dst = (Circle*)elem;
        break;
      }else{
        curIndex++;
      }
    }

    if(dst == NULL){
      printf("\nFrom setAttribute: Index %d out of bounds.", index);
      return;
    }

    //case 1: attribute is a field of the struct 
    if(strcmp(name, "cx") == 0){
      dst->cx = strtof(value, NULL);
      parseUnit(value, dst->units);
      (dst->units)[49] = '\0';
      caseExed = 1;
    }else if(strcmp(name, "cy") == 0){
      dst->cy = strtof(value, NULL);
      parseUnit(value, dst->units);
      (dst->units)[49] = '\0';
      caseExed = 1;
    }else if(strcmp(name, "r") == 0){
      dst->r = strtof(value, NULL);
      parseUnit(value, dst->units);
      (dst->units)[49] = '\0';
      caseExed = 1;
    }else if(strcmp(name, "units") == 0){
      parseUnit(value, dst->units);
      (dst->units)[49] = '\0';
      caseExed = 1;
    }

    //case 2: attr replaces member of otherAttributes list 
    if(caseExed == 0){
      if(sa_attrReplaced(dst->otherAttributes, newAttribute) == 1){
        caseExed = 1;
      }
    }

    //case 3: entirely new attribute in otherAttributes
    if(caseExed == 0){
      insertBack(dst->otherAttributes, newAttribute);
      caseExed = 3;
    }

  if(caseExed != 3)
    sa_freeAttr(newAttribute);
}

void setAttributeSVG(SVGimage* img, Attribute* toAdd){
  //preliminary check
  if(img == NULL || sa_preliminaryCheckAttr(toAdd) == 0){
    printf("\ninvalid attribute");
    return;
  }
  char* name = toAdd->name;
  char* value = toAdd->value;


  int caseExed = 0; // case = 0 if none of the cases have executed yet
  //Case 1: attribute is preexisting field of img 
  if(strcmp(name, "namespace") == 0){
    strncpy(img->namespace, value, 255);
    (img->namespace)[255] = '\0';
    caseExed = 1;
  }else if(strcmp(name, "title") == 0){
    strncpy(img->title, value, 255);
    (img->title)[255] = '\0';
    caseExed = 1;
  }else if(strcmp(name, "description") == 0){
    strncpy(img->description, value, 255);
    (img->description)[255] = '\0';
    caseExed = 1;
  }
    
  //Case 2: attribute is existing member of otherAttributes
  if(caseExed == 0){
    if(sa_attrReplaced(img->otherAttributes, toAdd) == 1){
      caseExed = 1;
    }
  }
  
  //Case 3: new attr
  if(caseExed == 0){
    insertBack(img->otherAttributes, toAdd);
    caseExed = 3;
  }
  if(caseExed != 3){
    sa_freeAttr(toAdd); 
  }
}
int sa_attrReplaced(List* otherAttributes, Attribute* toAdd){
  char* name = toAdd->name;
  char* value = toAdd->value;
  ListIterator iter = createIterator(otherAttributes);
  void* elem; 
  
  while ((elem = nextElement(&iter)) != NULL){
    Attribute* attr = (Attribute*)elem;
    if(strcmp(attr->name, name) == 0){
      attr->value = realloc(attr->value, sizeof(char) * (strlen(value)+1));
      strcpy(attr->value, value);
      return 1;
    }
  }

    return 0;
}
//return 0 if invalid attr, 1 if valid 
int sa_preliminaryCheckAttr(Attribute* newAttribute){
  if(newAttribute == NULL)
    return 0;
  if(newAttribute->name == NULL){
    return 0;
  }
  if(newAttribute->value == NULL){
    return 0;
  }
  return 1;
}
void sa_freeAttr(Attribute* toAdd){
  if(toAdd->value)
    free(toAdd->value);
  
  if(toAdd->name)
    free(toAdd->name);
  
  if(toAdd)
    free(toAdd);
}
void sa_addNewAttr(List* otherAttributes, Attribute* toAdd){
    insertBack(otherAttributes, (void*)toAdd);
}
/*************************helper fx MODULE 2.1*************************************/
xmlDoc* parseTree(SVGimage* img){
  //init + root
  xmlDoc* doc = xmlNewDoc(BAD_CAST "1.0");
  xmlNode* root = xmlNewNode(NULL, BAD_CAST "svg");

  //namespace, title, desc 
  xmlNsPtr ns = xmlNewNs(root, (xmlChar*)(img->namespace), NULL);
  xmlSetNs(root, ns);
  addOtherAttr(root, img->otherAttributes);

  if(strlen(img->title) > 0)
    xmlNewChild(root, NULL, (xmlChar*)"title", (xmlChar*)(img->title));
 
  if(strlen(img->description) > 0)
    xmlNewChild(root, NULL, (xmlChar*)"desc", (xmlChar*)(img->description));
  
  //populate root  
  addChildRects(root, img->rectangles);
  addChildCircs(root, img->circles);
  addChildPaths(root, img->paths);
  addChildGroups(root, img->groups);
  xmlDocSetRootElement(doc, root);
  
  return doc;
}
void addChildGroups(xmlNode* dst, List* groups){
  ListIterator iter = createIterator(groups);
  void* elem;

  //iterate through list; add all rects to tree
  while ((elem = nextElement(&iter)) != NULL){
    Group* group = (Group*)elem;
    xmlNode* addedNode = xmlNewChild(dst, NULL, (xmlChar*)"g", NULL);

    //add all children ele 
    addChildRects(addedNode, group->rectangles);
    addChildCircs(addedNode, group->circles);
    addChildPaths(addedNode, group->paths);
    addChildGroups(addedNode, group->groups);

    //otherattr 
    addOtherAttr(addedNode, group->otherAttributes);
  }
}

void addChildPaths(xmlNode* dst, List* paths){
  ListIterator iter = createIterator(paths);
  void* elem;

  //iterate through list; add all rects to tree
  while ((elem = nextElement(&iter)) != NULL){
    Path* path = (Path*)elem;
    xmlNode* addedNode = xmlNewChild(dst, NULL, (xmlChar*)"path", NULL);

    xmlNewProp(addedNode, BAD_CAST "d", BAD_CAST path->data);
    //otherattr 
    addOtherAttr(addedNode, path->otherAttributes);
  }
}
//adds otherattr to node arg
void addOtherAttr(xmlNode* node, List* attr){
  ListIterator iter = createIterator(attr);
  void* elem;

  //iterate through list; add all rects to tree
  while ((elem = nextElement(&iter)) != NULL){
    Attribute* attr = (Attribute*)elem;
    xmlNewProp(node, BAD_CAST attr->name, BAD_CAST attr->value);
  }
}
void addChildCircs(xmlNode* dst, List* circs){
  ListIterator iter = createIterator(circs);
  void* elem;
  char* buffer = malloc(50 * sizeof(char));
  //iterate through list; add all rects to tree
  while ((elem = nextElement(&iter)) != NULL){
    Circle* circ = (Circle*)elem;
    xmlNode* addedNode = xmlNewChild(dst, NULL, (xmlChar*)"circle", NULL);

    //cx
    snprintf(buffer, 15, "%f", circ->cx);
    smartConcat(&buffer, circ->units);
    xmlNewProp(addedNode, BAD_CAST "cx", BAD_CAST buffer);

    //y
    snprintf(buffer, 15, "%f", circ->cy);
    smartConcat(&buffer, circ->units);
    xmlNewProp(addedNode, BAD_CAST "cy", BAD_CAST buffer);

    //r
    snprintf(buffer, 15, "%f", circ->r);
    smartConcat(&buffer, circ->units);
    xmlNewProp(addedNode, BAD_CAST "r", BAD_CAST buffer);

    //otherattr 
    addOtherAttr(addedNode, circ->otherAttributes);
  }
  free(buffer);
}
void addChildRects(xmlNode* dst, List* rects){
  ListIterator iter = createIterator(rects);
  void* elem;
  char* buffer = malloc(50 * sizeof(char));

  //iterate through list; add all rects to tree
  while ((elem = nextElement(&iter)) != NULL){
    Rectangle* rect = (Rectangle*)elem;
    xmlNode* addedRect = xmlNewChild(dst, NULL, (xmlChar*)"rect", NULL);

    //rect->x
    snprintf(buffer, 15, "%f", rect->x);
    smartConcat(&buffer, rect->units);
    xmlNewProp(addedRect, BAD_CAST "x", BAD_CAST buffer);

    //rect->y
    snprintf(buffer, 15, "%f", rect->y);
    smartConcat(&buffer, rect->units);
    xmlNewProp(addedRect, BAD_CAST "y", BAD_CAST buffer);

    //rect->width
    snprintf(buffer, 15, "%f", rect->width);
    smartConcat(&buffer, rect->units);
    xmlNewProp(addedRect, BAD_CAST "width", BAD_CAST buffer);

    //rect->height
    snprintf(buffer, 15, "%f", rect->height);
    smartConcat(&buffer, rect->units);
    xmlNewProp(addedRect, BAD_CAST "height", BAD_CAST buffer);

    //otherattr 
    addOtherAttr(addedRect, rect->otherAttributes);
  }
  free(buffer);
}
//check if filename is terminated with .svg
bool hasFileExt(char* str){
  char ext[5] = {'.', 's', 'v', 'g', '\0'};
  int j = 0;
  if(strlen(str) < 4){
    return false;
  }

  for(int i = strlen(str) - 4; i < strlen(str); i++){
    if(str[i] != ext[j]){
      return false;
    }
    j++;
  }
  return true;
}

/*********************STUBS*********************/
char* attrToJSON(const Attribute*);
char* groupToJSON(const Group *);
char* pathToJSON(const Path *);
char* rectToJSON(const Rectangle *);
char* circleToJSON(const Circle *);

Attribute* JSONtoAttribute(const char*);
Circle* JSONtoCircle(const char*);
Rectangle* JSONtoRect(const char*);

//overwrite title/desc of file; return = 1 for success, 0 for fail
int setTDFile(char* fname, const char* json){
  SVGimage* svg = createValidSVGimage(fname, "parser/svg.xsd");
  if(!svg){
    return 0;
  }
  char** vals = valuesJSON(json);

  strncpy(svg->title, vals[0], 255);
  strncpy(svg->description, vals[1], 255);
  if(!validateSVGimage(svg, "parser/svg.xsd")){
    return 0;
  }
  writeSVGimage(svg, vals[2]);
  //printf("\nt(%s) d(%s)", svg->title, svg->description);
  free(vals[0]);
  free(vals[1]);
  free(vals[2]);
  free(vals);
  return 1;
}
//set attribute in file; overwrite exising IF VALID. return 1 for success, 0 for failure
int setAttrFile(char* fname, char* json, elementType elemType, int elemIndex){
  SVGimage* svg = createValidSVGimage(fname, "parser/svg.xsd");
  if(!svg){
    return 0;
  }
  Attribute *new_attr = JSONtoAttribute(json);
  setAttribute(svg, elemType, elemIndex, new_attr);
  if(validateSVGimage(svg, "parser/svg.xsd")==false){
    return -1;
  }

  writeSVGimage(svg, fname);
  free(svg);
  return 1;
}
//takes svg json, returns array of its values ORDERED IN STRING; NO ERRORCHECKING
char** valuesJSON(const char *svgString){
  char** toReturn = malloc(sizeof(char*));
  
  int numVals =0;
  char buffer[1000] = ""; 
  int len = strlen(svgString);
  int copy = 0;
  int cpyIdx=0;
  for(int i = 0; i < len; i++){
    if(svgString[i]==':'){
      i++;
      copy = 1;
      numVals++;
      cpyIdx=0;
      if(svgString[i]=='\"'){
        i++;
      }
    }
    if(copy && (svgString[i] == '\"'|| svgString[i] == ',')){
      buffer[cpyIdx] = '\0';
      toReturn = realloc(toReturn, sizeof(char*) *numVals);
      toReturn[numVals-1] = malloc(strlen(buffer) + 1);
      strcpy(toReturn[numVals-1], buffer);
      copy = 0;
      //printf("\nstring = [%s]", buffer);
    }else if (copy){
      buffer[cpyIdx] = svgString[i];
      cpyIdx++;
    }
  }
  
  return toReturn;
  
}
Attribute* JSONtoAttribute(const char* attrString){
  char** vals = valuesJSON(attrString);
  Attribute *toReturn = malloc(sizeof(Attribute));
  toReturn->name = vals[0];
  toReturn->value = vals[1];
  free(vals);
  return toReturn;
}
SVGimage* JSONtoSVG(const char* svgString){
  SVGimage* toReturn = malloc(sizeof(SVGimage));
  char buffJSON[1000] = "";
  strcpy(buffJSON, svgString);
  
  //init 
  strcpy(toReturn->namespace, "http://www.w3.org/2000/svg"); 
  strcpy(toReturn->title, "");
  strcpy(toReturn->description, "");

  initLists(toReturn);
  char** x = valuesJSON(svgString);
  strcpy(toReturn->title, x[0]);
  strcpy(toReturn->description,x[1]);

  free(x[0]);
  free(x[1]);
  free(x);
  return toReturn;
}
Rectangle* JSONtoRect(const char* svgString){
  //{"x":xVal,"y":yVal,"w":wVal,"h":hVal,"units":"unitStr"}
  Rectangle* toReturn = malloc(sizeof(Rectangle));
  toReturn->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);
  
  char** vals = valuesJSON(svgString);
  toReturn->x = strtof(vals[0], NULL);
  toReturn->y = strtof(vals[1], NULL);
  toReturn->width = strtof(vals[2], NULL);
  toReturn->height = strtof(vals[3], NULL);
  strcpy(toReturn->units, vals[4]);

  for(int i = 0; i < 5; i++)
    free(vals[i]);
  free(vals);

  return toReturn;
}
Circle* JSONtoCircle(const char* svgString){
  //{"cx":xVal,"cy":yVal,"r":rVal,"units":"unitStr"}
  Circle* toReturn = malloc(sizeof(Circle));
  toReturn->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);

  char** vals = valuesJSON(svgString);
  toReturn->cx = strtof(vals[0], NULL);
  toReturn->cy =strtof(vals[1], NULL);
  toReturn->r = strtof(vals[2], NULL);
  strcpy(toReturn->units, vals[3]);

  for(int i = 0; i < 4; i++)
    free(vals[i]);
  free(vals);
  //printf("\n%s", circleToString(toReturn));
  return toReturn;
}
char* SVGtoJSON(const SVGimage* imge){
  char* toReturn;
  char buffer[2000] = "";
  if(imge == NULL){
    toReturn = malloc(3 * sizeof(char));
    strcpy(toReturn, "{}");
    return toReturn;
  }
  int numR, numC, numP, numG;
  List* allR, *allC, *allP,*allG;

  allR = getRects((SVGimage*)imge);
  allC = getCircles((SVGimage*)imge);
  allP = getPaths((SVGimage*)imge);
  allG = getGroups((SVGimage*)imge);

  numR = listLength(allR);
  numC = listLength(allC);
  numP = listLength(allP);
  numG = listLength(allG);

  freeList(allR);
  freeList(allC);
  freeList(allP);
  freeList(allG);

  sprintf(buffer, "{\"numRect\":%d,\"numCirc\":%d,\"numPaths\":%d,\"numGroups\":%d}", numR, numC, numP, numG);
  toReturn = malloc(sizeof(char) * (strlen(buffer) + 1));
  strcpy(toReturn, buffer);

  return toReturn;
}
char* groupListToJSON(const List *list){
  char* toReturn;
  if(list == NULL){
    toReturn = malloc(3 * (sizeof(char) + 1));
    strcpy(toReturn, "[]");
    return toReturn;
  }

  toReturn = malloc(2);
  strcpy(toReturn, "[");
  ListIterator iter = createIterator((List*)list);
  
  void* elem = nextElement(&iter);
  char* str = NULL;
  //first iteration: account for commas 
  if(elem != NULL){
    str = groupToJSON(elem);
    smartConcat(&toReturn, str);
    safeFree(str);
  }
  while ((elem = nextElement(&iter)) != NULL){
    smartConcat(&toReturn, ",");
    str = groupToJSON(elem);
    smartConcat(&toReturn, str);
    safeFree(str);
  }
  smartConcat(&toReturn, "]");
  return toReturn;
}
char* pathListToJSON(const List *list){
  char* toReturn;
  if(list == NULL){
    toReturn = malloc(3 * (sizeof(char) + 1));
    strcpy(toReturn, "[]");
    return toReturn;
  }

  toReturn = malloc(2);
  strcpy(toReturn, "[");
  ListIterator iter = createIterator((List*)list);
  
  void* elem = nextElement(&iter);
  char* str = NULL;
  //first iteration: account for commas 
  if(elem != NULL){
    str = pathToJSON(elem);
    smartConcat(&toReturn, str);
    safeFree(str);
  }
  while ((elem = nextElement(&iter)) != NULL){
    smartConcat(&toReturn, ",");
    str = pathToJSON(elem);
    smartConcat(&toReturn, str);
    safeFree(str);
  }
  smartConcat(&toReturn, "]");
  return toReturn;
}
char* rectListToJSON(const List *list){
  char* toReturn;
  if(list == NULL){
    toReturn = malloc(3 * (sizeof(char) + 1));
    strcpy(toReturn, "[]");
    return toReturn;
  }

  toReturn = malloc(2);
  strcpy(toReturn, "[");
  ListIterator iter = createIterator((List*)list);
  
  void* elem = nextElement(&iter);
  char* str = NULL;
  //first iteration: account for commas 
  if(elem != NULL){
    str = rectToJSON(elem);
    smartConcat(&toReturn, str);
    safeFree(str);
  }
  while ((elem = nextElement(&iter)) != NULL){
    smartConcat(&toReturn, ",");
    str = rectToJSON(elem);
    smartConcat(&toReturn, str);
    safeFree(str);
  }
  smartConcat(&toReturn, "]");
  return toReturn;
}
char* circListToJSON(const List *list){
  char* toReturn;
  if(list == NULL){
    toReturn = malloc(3 * (sizeof(char) + 1));
    strcpy(toReturn, "[]");
    return toReturn;
  }

  toReturn = malloc(2);
  strcpy(toReturn, "[");
  ListIterator iter = createIterator((List*)list);
  
  void* elem = nextElement(&iter);
  char* str = NULL;
  //first iteration: account for commas 
  if(elem != NULL){
    str = circleToJSON(elem);
    smartConcat(&toReturn, str);
    safeFree(str);
  }
  while ((elem = nextElement(&iter)) != NULL){
    smartConcat(&toReturn, ",");
    str = circleToJSON(elem);
    smartConcat(&toReturn, str);
    safeFree(str);
  }
  smartConcat(&toReturn, "]");
  return toReturn;
}
char* attrListToJSON(const List *list){
  char* toReturn;
  if(list == NULL){
    toReturn = malloc(3 * (sizeof(char) + 1));
    strcpy(toReturn, "[]");
    return toReturn;
  }

  toReturn = malloc(2);
  strcpy(toReturn, "[");
  ListIterator iter = createIterator((List*)list);
  
  void* elem = nextElement(&iter);
  char* str = NULL;
  //first iteration: account for commas 
  if(elem != NULL){
    str = attrToJSON(elem);
    smartConcat(&toReturn, str);
    safeFree(str);
  }
  while ((elem = nextElement(&iter)) != NULL){
    smartConcat(&toReturn, ",");
    str = attrToJSON(elem);
    smartConcat(&toReturn, str);
    safeFree(str);
  }
  smartConcat(&toReturn, "]");
  return toReturn;
}
char* groupToJSON(const Group *g){
  char *toReturn;
  char buffer[2000];
  int numChildren;

  if(g == NULL){
    toReturn = malloc(sizeof(char) * 3);
    strcpy(toReturn, "{}");
    return toReturn;    
  }

  numChildren = listLength(g->rectangles) + listLength(g->circles) + listLength(g->paths) + listLength(g->groups);
  sprintf(buffer, "{\"children\":%d,\"numAttr\":%d}", numChildren, listLength(g->otherAttributes));
  toReturn = malloc(sizeof(char) * (strlen(buffer) + 1));
  strcpy(toReturn, buffer);

  return toReturn;
}
char* pathToJSON(const Path *p){
  char* toReturn;
  char buffer[2000];
  char dBuffer[65];

  if(p == NULL){
    toReturn = malloc(sizeof(char) * 3);
    strcpy(toReturn, "{}");
    return toReturn;
  }
  strcpy(dBuffer, p->data);
  dBuffer[64] = '\0';
  
  sprintf(buffer, "{\"d\":\"%s\",\"numAttr\":%d}", dBuffer,listLength(p->otherAttributes));
  toReturn = malloc(sizeof(char) * (strlen(buffer) + 1));
  strcpy(toReturn, buffer);

  return toReturn;
}
char* rectToJSON(const Rectangle *r){
  char* toReturn;
  char buffer[2000];

  if(r == NULL){
    toReturn = malloc(sizeof(char) * 3);
    strcpy(toReturn, "{}");
    return toReturn;
  }
  sprintf(buffer, "{\"x\":%.2f,\"y\":%.2f,\"w\":%.2f,\"h\":%.2f,\"numAttr\":%d,\"units\":\"%s\"}", r->x, r->y, r->width, r->height, listLength(r->otherAttributes), r->units);
  toReturn = malloc(sizeof(char) * (strlen(buffer) + 1));
  strcpy(toReturn, buffer);

  return toReturn;
}
char* circleToJSON(const Circle *c){
  char* toReturn;
  char buffer[2000];

  if(c == NULL){
    toReturn = malloc(3*sizeof(char));
    strcpy(toReturn, "{}");
    return toReturn;
  }

  sprintf(buffer, "{\"cx\":%.2f,\"cy\":%.2f,\"r\":%.2f,\"numAttr\":%d,\"units\":\"%s\"}", c->cx, c->cy, c->r, listLength(c->otherAttributes), c->units);
  toReturn = malloc(sizeof(char) * (strlen(buffer) + 1));
  strcpy(toReturn, buffer);
  return toReturn;
}
char* attrToJSON(const Attribute *a){
  char* toReturn;
  char buffer[2023];

  if(a == NULL || a->name == NULL || strlen(a->name) == 0){
    toReturn = malloc(sizeof(char) * 3);
    strcpy(toReturn, "{}");
    return toReturn;
  }
  sprintf(buffer, "{\"name\":\"%s\",\"value\":\"%s\"}", a->name, a->value);
  
  toReturn = malloc(sizeof(char) * (strlen(buffer) + 1));
  strcpy(toReturn, buffer);

  return toReturn;
}
/***********************crap from a1***************************/
void safeFree(void* data){
  if(data != NULL){
    free(data);
  }
}
//parse unit into string byref
void parseUnit(char* str, char* copyInto){
  if(strlen(copyInto) != 0)
    return;

  char buffer[strlen(str) + 1];
  int bIndex = 0;
  for(int i = 0; i < strlen(str); i++){
    if(!isdigit(str[i]) && str[i] != '.' && str[i] != ' ' &&str[i] != '-'){
      buffer[bIndex] = str[i];
      bIndex++;
    }
  }
  buffer[bIndex] = '\0';

  if(strlen(buffer) >= 49){
    buffer[49] = '\0';
  }
  if(strcmp(buffer, "") != 0)
    strcpy(copyInto, buffer);
}
//CUSTOM HELPER FUNCTIONS ----------------------------------------------------------------------------
//parses ONE attribute (obviously, look at the return type IDIOT!)
Attribute* parseAttribute(xmlAttr* attr){
  Attribute* toReturn = malloc(sizeof(Attribute));

  int nameLength = strlen((char*)attr->name);
  toReturn->name = malloc(sizeof(char) * (nameLength + 1));
  strcpy(toReturn->name, (char*)attr->name);

  int valLength = strlen((char*)(attr->children)->content);
  toReturn->value = malloc(sizeof(char) * (valLength +1));
  strcpy(toReturn->value, (char*)(attr->children)->content);

  return toReturn;
}
void freeAttribute(Attribute* toFree){
  safeFree(toFree->name);
  safeFree(toFree->value);
  safeFree(toFree);
}
//explore all the current group's children, adding child groups to toReturn->group and exploring child groups' children
void defineGroup(xmlNode* a_node, Group* parentGroup, int depth){
  xmlNode* cur_child = a_node->children;

  //iterate through children of the group
  for(;cur_child != NULL; cur_child = cur_child->next){

    if(!strcmp((char*)(cur_child->name), "g")){
      //add child group to parentgroup->groups id existing
      insertBack(parentGroup->groups, allocGroup(cur_child));

      //go scan group's children for more children
      if(cur_child->children){
        if(!parentGroup->groups){
          parentGroup->groups = initializeList(&groupToString, &deleteGroup, &compareGroups);
        }
        defineGroup(cur_child, getFromFront(parentGroup->groups), depth + 1);
      }
    }
  }
}
//allocate group + lists, parse node for non-group properties
Group* allocGroup(xmlNode* cnode){
    Group* toReturn = malloc(sizeof(Group));
    //init lists
    toReturn->rectangles = initializeList(&rectangleToString, &deleteRectangle, &compareRectangles);
    toReturn->circles = initializeList(&circleToString, &deleteCircle, &compareCircles);
    toReturn->paths = initializeList(&pathToString, &deletePath, &comparePaths);
    toReturn->groups = initializeList(&groupToString, &deleteGroup, &compareGroups);
    toReturn->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);

    //iterate through all CHILDREN ONLY node of the group, add to PROPERTIES
    for(xmlNode* node = cnode->children; node != NULL; node=node->next){

      if(!strcmp((char*)(node->name), "circle")){
        Circle* circle = parseCircle(node);
        insertBack(toReturn->circles, (void*)circle);
      }
      if(!strcmp((char*)(node->name), "rect")){
        Rectangle *r = parseRectangle(node);
        insertBack(toReturn->rectangles, (void*)r);
      }
      if(!strcmp((char*)(node->name), "path")){
        Path* p = parsePath(node);
        insertBack(toReturn->paths, p);
      }

    }
    //parse de attributes
    for(xmlAttr* a = cnode->properties; a != NULL; a = a->next){
      Attribute* new_a = parseAttribute(a);
      insertBack(toReturn->otherAttributes, new_a);
    }

    return toReturn;
}

Circle* parseCircle(xmlNode *node){
  //INITIALIZE EVERYTHING
  Circle* toReturn = malloc(sizeof(Circle));
  toReturn->cx = 0;
  toReturn->cy = 0;
  toReturn->r = 0;
  toReturn->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);
  strcpy(toReturn->units, "");

  xmlAttr* attr;
  for(attr = node->properties; attr!=NULL; attr = attr->next){
      //parse x
    if(!strcmp("cx", (char*)attr->name)){
      float cxfloat = strtof( (char*)(attr->children)->content, NULL);
      toReturn->cx = cxfloat;

      parseUnit((char*)(attr->children)->content, toReturn->units);
    }
    //parse cy
    else if(!strcmp("cy", (char*)attr->name)){
      float cyfloat = strtof( (char*)(attr->children)->content, NULL);
      toReturn->cy = cyfloat;

      parseUnit((char*)(attr->children)->content, toReturn->units);
    }
    //parse radius
    else if(!strcmp("r", (char*)attr->name)){
      float rfloat = strtof( (char*)(attr->children)->content, NULL);
      toReturn->r =rfloat;

      parseUnit((char*)(attr->children)->content, toReturn->units);
    }
    else{
      Attribute* newAttr =parseAttribute(attr);
      insertBack(toReturn->otherAttributes, (void*)newAttr);
    }
  }
  return toReturn;
}

Path* parsePath(xmlNode *node){
  //INIT
  Path* toReturn = malloc(sizeof(Path));
  toReturn->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);
  toReturn->data = malloc(sizeof(char)*2);
  strcpy(toReturn->data, "");

  //PARSE STRUCT PROPERTIES
  xmlAttr* attr;
  for(attr = node->properties; attr!=NULL; attr = attr->next){
    //parse data attr
    if(!strcmp("d", (char*)attr->name)){
      int strlength = strlen((char*)((attr->children)->content));
      toReturn->data = realloc(toReturn->data, sizeof(char) * (strlength+ 1));
      strcpy(toReturn->data, (char*)((attr->children)->content));
    }else{
      Attribute* newAttr = parseAttribute(attr);
      insertBack(toReturn->otherAttributes, newAttr);
    }
  }

  return toReturn;
}


Rectangle* parseRectangle(xmlNode *node){
  //INIT
  Rectangle* toReturn = malloc(sizeof(Rectangle));
  toReturn->width = 0;
  toReturn->height = 0;
  toReturn->x = 0;
  toReturn->y = 0;
  strcpy(toReturn->units, "");
  toReturn->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);

  //PARSE PROPERTIES
  xmlAttr* attr;
  for(attr = node->properties; attr!=NULL; attr = attr->next){
      //parse x
    if(!strcmp("x", (char*)attr->name)){
      float xfloat = strtof( (char*)(attr->children)->content, NULL);
      toReturn->x = xfloat;
      parseUnit((char*)(attr->children)->content, toReturn->units);
    }
      //parse cy
    else if(!strcmp("y", (char*)attr->name)){
      float yfloat = strtof( (char*)(attr->children)->content, NULL);
      toReturn->y = yfloat;
      parseUnit((char*)(attr->children)->content, toReturn->units);
    }
      //parse radius
    else if(!strcmp("width", (char*)attr->name)){
      float wfloat = strtof( (char*)(attr->children)->content, NULL);
      toReturn->width =wfloat;
      parseUnit((char*)(attr->children)->content, toReturn->units);
    }
      //parse height
    else if(!strcmp("height", (char*)attr->name)){
      float hfloat = strtof( (char*)(attr->children)->content, NULL);
      toReturn->height =hfloat;
      parseUnit((char*)(attr->children)->content, toReturn->units);
    }else{
      Attribute* newAttr =parseAttribute(attr);
      insertBack(toReturn->otherAttributes, (void*)newAttr);
    }

  }

  return toReturn;
}

//depth first traversal of tree; define properties at every node
void defineTree(xmlNode * a_node, SVGimage *toReturn)
{
  xmlNode *cur_node = NULL;
  for (cur_node = a_node; cur_node != NULL; cur_node = cur_node->next) {
    //define current node
    if(cur_node->type == 1){
      strncpy(toReturn->namespace, (char*)((cur_node->ns)->href), 255);
      (toReturn->namespace)[255] ='\0';
    }
    //define title if existing
    if(strcmp( (char*)(cur_node->name), "title") == 0){
      strncpy(toReturn->title, (char*)(cur_node->children->content), 255);
      (toReturn->title)[255] ='\0';
    }
    //define desc if existing
    if(strcmp( (char*)(cur_node->name), "desc") == 0){
      strncpy(toReturn->description, (char*)(cur_node->children->content), 256);
      (toReturn->description)[255]='\0';
    }
    //groops
    if(strcmp((char*)(cur_node->name), "g") == 0){
      Group* g = allocGroup(cur_node);
      defineGroup(cur_node, g, 0);
      insertBack(toReturn->groups, (void*)g);
      continue;
    }
    if(!strcmp((char*)(cur_node->name), "circle")){
      Circle* circle = parseCircle(cur_node);
      insertBack(toReturn->circles, (void*)circle);
    }
    if(!strcmp((char*)(cur_node->name), "rect")){
      Rectangle *r = parseRectangle(cur_node);
      insertBack(toReturn->rectangles, (void*)r);
    }
    if(!strcmp((char*)(cur_node->name), "path")){
      Path* p = parsePath(cur_node);
      insertBack(toReturn->paths, p);
    }
      defineTree(cur_node->children, toReturn);
    }
}
//concats src to dst while resizing dst
void smartConcat(char** dst, char* src){
  //strlen(*dst);
  int len = 1;

  if((*dst) != NULL && src!=NULL){
    len = strlen(*dst) + strlen(src) + 2;
    *dst = realloc(*dst, len * sizeof(int));
    strcat(*dst, src);
  }
}
/** Function to create a string representation of an SVG object.
 *@pre SVGimgage exists, is not null, and is valid
 *@post SVGimgage has not been modified in any way, and a string representing the SVG contents has been created
 *@return a string contaning a humanly readable representation of an SVG object
 *@param obj - a pointer to an SVG struct
**/
char* SVGimageToString(SVGimage* img){
  if(img == NULL)
    return "";

  char buffer[1000] = "";
  char buffer1[400];

  char** toReturn = NULL;
  toReturn = malloc(sizeof(char*));
  *toReturn = malloc(5);
  strcpy(*toReturn, "");

  sprintf(buffer1,"\n\n*SUMMARY");
  strcat(buffer, buffer1);

  sprintf(buffer1, "\nNamespace: %s", img->namespace);
  strcat(buffer, buffer1);

  sprintf(buffer1, "\nTitle: %s", img->title);
  strcat(buffer, buffer1);

  sprintf(buffer1, "\nDesc: %s\nOther Attributes:", img->description);
  strcat(buffer, buffer1);

  smartConcat(toReturn, buffer);

  ListIterator iter = createIterator(img->otherAttributes);
  void* elem;
  char* str = NULL;
  //segfaults
  while ((elem = nextElement(&iter)) != NULL){
    str = attributeToString(elem);
    smartConcat(toReturn, str);
    smartConcat(toReturn, "\n");

    safeFree(str);
  }
  

  iter = createIterator(img->rectangles);
  //rects
  while ((elem = nextElement(&iter)) != NULL){
    Rectangle* tmp = (Rectangle*)elem;
    str = (img->rectangles)->printData(tmp);
    smartConcat(toReturn, str);
    //printf("%s\n", str);

    safeFree(str);
  }

  iter = createIterator(img->circles);
  //circles
  while ((elem = nextElement(&iter)) != NULL){
    Circle* tmp = (Circle*)elem;
    str = (img->circles)->printData(tmp);
    smartConcat(toReturn, str);

    //printf("%s\n", str);

    safeFree(str);
  }

  iter = createIterator(img->paths);
  while ((elem = nextElement(&iter)) != NULL){
    Path* tmp = (Path*)elem;
    str = (img->paths)->printData(tmp);
    smartConcat(toReturn, str);

    //printf("%s\n", str);

    safeFree(str);
  }

  iter = createIterator(img->groups);
  while ((elem = nextElement(&iter)) != NULL){
    Group* tmp = (Group*)elem;
    str = (img->groups)->printData(tmp);
    smartConcat(toReturn, str);

    safeFree(str);
  }

  char* strPtr = malloc(sizeof(char) * (strlen(*toReturn) + 1));
  strcpy(strPtr, *toReturn);
  free(*toReturn);
  free(toReturn);

  return strPtr;
}

void initLists(SVGimage* toReturn){
  toReturn->rectangles = initializeList(&rectangleToString, &deleteRectangle, &compareRectangles);
  toReturn->circles = initializeList(&circleToString, &deleteCircle, &compareCircles);
  toReturn->paths = initializeList(&pathToString, &deletePath, &comparePaths);

  toReturn->groups = initializeList(&groupToString, &deleteGroup, &compareGroups);
  toReturn->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);
}

//END OF CUSTOM HELPER FUNCTIONS ----------------------------------------------------------------------------


/*HELPER FUNCTIONS------------------------------------------------------------*/
void deleteAttribute(void* data){
  if(!data){
    return;
  }
  safeFree(((Attribute*)data)->name);
  safeFree(((Attribute*)data)->value);
 safeFree(data);
}
char* attributeToString( void* data){
  if(!data){
    return NULL;
  }

  char buffer[400];
  Attribute *attr = (Attribute*)data;
  sprintf(buffer, "\n\t%s = %s", attr->name, attr->value);

  char *toReturn = malloc(sizeof(char) * (strlen(buffer) + 1));
  strcpy(toReturn, buffer);
  return toReturn;
}
int compareAttributes(const void *first, const void *second){
  return 100;
}

void defectiveDelete(){

}
//for every group in this list, delete its children and grandchildren etc. groups too.
void deleteGroup(void* data){
  if(!data){
    return;
  }
  Group* group = (Group*)data;

  if(group->rectangles)
    freeList(group->rectangles);
  if(group->circles)
    freeList(group->circles);
  if(group->paths)
    freeList(group->paths);
  if(group->otherAttributes)
    freeList(group->otherAttributes);

  //recursively search for deepest group to delete first
  if(group->groups != NULL){
    //printf("\ndirect child...\n");
    freeList(group->groups);
  }
  safeFree(data);
}
void smartCpy(char** dst, char* src){

  if((*dst) == NULL){
    *dst = malloc(sizeof(char) * (strlen(src) + 1 ));
  }else{
    *dst = realloc(*dst, sizeof(char) * (strlen(*dst) + strlen(src) + 1));
  }
  strcpy(*dst, src);
}

char* groupToString( void* data){
  if(!data){
    return NULL;
  }
  Group* g = (Group*)data;
  char* buffer = malloc(1000);
  char buffer2[500] = "";
  sprintf(buffer, "\n\n\n*************************Group*************************\n");

  //rects
  ListIterator iter = createIterator(g->rectangles);

  void* elem;
  //rects
  while ((elem = nextElement(&iter)) != NULL){
    Rectangle* tmp = (Rectangle*)elem;
    char* str = (g->rectangles)->printData(tmp);
    smartConcat(&buffer, str);
    safeFree(str);
  }

  iter = createIterator(g->circles);
  while ((elem = nextElement(&iter)) != NULL){
    Circle* tmp = (Circle*)elem;
    char* str = (g->circles)->printData(tmp);
    smartConcat(&buffer, str);
    safeFree(str);
  }

  iter = createIterator(g->paths);
  while ((elem = nextElement(&iter)) != NULL){
    Path* tmp = (Path*)elem;
    char* str = (g->paths)->printData(tmp);
    smartConcat(&buffer, str);
    safeFree(str);
  }

  iter = createIterator(g->otherAttributes);
  while ((elem = nextElement(&iter)) != NULL){
    Attribute* tmp = (Attribute*)elem;
    char* str = (g->otherAttributes)->printData(tmp);
    smartConcat(&buffer, str);
    safeFree(str);
  }
  sprintf(buffer2, "\n\tChild Groups: %d\n", listLength(g->groups));
  smartConcat(&buffer, buffer2);
  smartConcat(&buffer, "\n***********************************************************\n\n\n");

  //return
  return buffer;
}

int compareGroups(const void *first, const void *second){
  return 100;
}
void deleteRectangle(void* data){
  if(!data){
    return;
  }
  Rectangle* rect = (Rectangle*)data;
  if(rect->otherAttributes){
    freeList(rect->otherAttributes);
  }
  safeFree(data);
}
char* rectangleToString(void* data){
  if(!data){
    return NULL;
  }

  //baysic parse
  Rectangle* r = (Rectangle*)data;
  char buffer[1000];
  char buffer2[300];
  sprintf(buffer, "\nRectangle***********************\nx = %f; y = %f; \nwidth = %f; height = %f; \nunits = '%s'\n\nOther Attributes:",
        r->x, r->y, r->width, r->height, r->units);

  //parse attr
  ListIterator iter = createIterator(r->otherAttributes);
  Attribute* ele;
  while((ele = (Attribute*)nextElement(&iter))!=NULL){
    sprintf(buffer2, "\n\t%s: %s", ele->name, ele->value);
    strcat(buffer, buffer2);
  }
  strcat(buffer, "\n********************************\n");

  //return
  char* toReturn = malloc(sizeof(char) * (strlen(buffer) + 1));
  strcpy(toReturn, buffer);
  return toReturn;

}
int compareRectangles(const void *first, const void *second){
  return 0;
}

void deleteCircle(void* data){
  if(!data){
    return;
  }
  Circle* circ = (Circle*)data;
  if(circ->otherAttributes){
    freeList(circ->otherAttributes);
  }
  safeFree(data);
}
char* circleToString(void* data){
  if(!data){
    return NULL;
  }

  //baysic parse
  Circle* c = (Circle*)data;
  char buffer[1000];
  char buffer2[300];
  sprintf(buffer, "\nCircle**************************\ncx = %f; cy = %f; r = %f; \nunits = '%s'\n\nOther Attributes:",
        c->cx, c->cy, c->r, c->units);

  //parse attr
  ListIterator iter = createIterator(c->otherAttributes);
  Attribute* ele;
  while((ele = (Attribute*)nextElement(&iter))!=NULL){
    sprintf(buffer2, "\n\t%s: %s", ele->name, ele->value);
    strcat(buffer, buffer2);
  }
  strcat(buffer, "\n********************************\n");

  //return
  char* toReturn = malloc(sizeof(char) * (strlen(buffer) + 1));
  strcpy(toReturn, buffer);
  return toReturn;

}
int compareCircles(const void *first, const void *second){
  return 0;
}

void deletePath(void* data){
  if(!data){
    return;
  }
  Path* p = (Path*)data;
  if(p->otherAttributes){
    freeList(p->otherAttributes);
  }
  if(p->data){
    safeFree(p->data);
  }
  safeFree(data);
}
char* pathToString(void* data){
  if(!data){
    return NULL;
  }

  //baysic parse
  Path *p = (Path*)data;
  char buffer[1000];
  char buffer2[300];
  sprintf(buffer, "\nPath****************************\ndata:\n%s\n\nOther Attributes:",
        p->data);

  //parse attr
  ListIterator iter = createIterator(p->otherAttributes);
  Attribute* ele;
  while((ele = (Attribute*)nextElement(&iter))!=NULL){
    sprintf(buffer2, "\n\t%s: %s", ele->name, ele->value);
    strcat(buffer, buffer2);
  }
  strcat(buffer, "\n********************************\n");

  //return
  char* toReturn = malloc(sizeof(char) * (strlen(buffer) + 1));
  strcpy(toReturn, buffer);
  return toReturn;

}
int comparePaths(const void *first, const void *second){
  return 9;
}

//ACTUAL FUNCTIOoooooooooooooooooooooooooooooooooNS
SVGimage* createSVGimage(char* fileName){
  //check if valid svg first
  xmlDoc* xmlFile = xmlReadFile(fileName, NULL, 0);
  if (xmlFile == NULL) {
      printf("Error: could not parse file %s\n", fileName);
      xmlCleanupParser();
      
      return NULL;
  }

  //parse into svgi
  xmlNode *xmlRoot = xmlDocGetRootElement(xmlFile);
  SVGimage* toReturn = malloc(sizeof(SVGimage));

  //init string
  strcpy(toReturn->namespace, "");
  strcpy(toReturn->title, "");
  strcpy(toReturn->description, "");
  initLists(toReturn);

  //traverse tree & define svgimage
  defineTree(xmlRoot, toReturn);
  xmlAttr* a;
  for(a = xmlRoot->properties; a != NULL; a = a->next){
    Attribute *newAttr = parseAttribute(a);
    insertBack(toReturn->otherAttributes, newAttr);
  }

  //free
  xmlFreeDoc(xmlFile);
  xmlCleanupParser();

  return toReturn;
}



/** Function to delete image content and free all the memory.
 *@pre SVGimgage  exists, is not NULL, and has not been freed
 *@post SVSVGimgageG  had been freed
 *@return none
 *@param obj - a pointer to an SVG struct
**/
void deleteSVGimage(SVGimage* img){
  if(img == NULL)
    return;

  if(img->rectangles !=NULL){
    freeList(img->rectangles);
  }
  if(img->circles !=NULL){
    freeList(img->circles);
  }
  if(img->paths !=NULL){
    freeList(img->paths);
  }
  if(img->groups!=NULL){
    freeList(img->groups);
  //  deleteGroup(img->groups);
  }
  if(img->otherAttributes!=NULL){
    freeList(img->otherAttributes);
  }

  safeFree(img);
}
void printGroupStructure(List* groups, int depth){
  ListIterator iter = createIterator(groups);
  void* elem;

  while ((elem = nextElement(&iter)) != NULL){
    Group* tmp = (Group*)elem;
    printf("\ngroup at depth %d", depth);
    if(tmp->groups)
      printGroupStructure(tmp->groups, depth+1);
  }
}
void digForRectangles(List* g, List* addTo){
  ListIterator iter = createIterator(g);
  void* elem;

  ListIterator iter_r;
  void* elem_r;

  while ((elem = nextElement(&iter)) != NULL){
    Group* tmp = (Group*)elem;
    iter_r = createIterator(tmp->rectangles);

    //add rectangles from curr group
    while((elem_r = nextElement(&iter_r)) != NULL){
      Rectangle* tmp = (Rectangle*)elem_r;
      insertBack(addTo, tmp);
    }

    if(tmp->groups)
      digForRectangles(tmp->groups, addTo);
  }
}
// Function that returns a list of all rectangles in the image.
List* getRects(SVGimage* img){
  List *toReturn = initializeList(&rectangleToString, &defectiveDelete, &compareRectangles);

  if(img == NULL || img->rectangles == NULL)
    return toReturn;

  ListIterator iter = createIterator(img->rectangles);

  void* elem;
  //rects in img->rects
  while ((elem = nextElement(&iter)) != NULL){
    Rectangle* tmp = (Rectangle*)elem;
    insertBack(toReturn, tmp);
  }

  //add rects in groups
  digForRectangles(img->groups, toReturn);

  return toReturn;
}

void digForCircles(List* g, List* addTo){
  ListIterator iter = createIterator(g);
  void* elem;

  ListIterator iter_c;
  void* elem_c;

  while ((elem = nextElement(&iter)) != NULL){
    Group* tmp = (Group*)elem;
    iter_c = createIterator(tmp->circles);

    //add rectangles from curr group
    while((elem_c = nextElement(&iter_c)) != NULL){
      Circle* tmp = (Circle*)elem_c;
      insertBack(addTo, tmp);
    }

    if(tmp->groups)
      digForCircles(tmp->groups, addTo);
  }
}
void digForGroups(List* g, List* addTo, int depth){
  ListIterator iter = createIterator(g);
  void* elem;
  ListIterator iter_g;
  void* elem_g;

  while ((elem = nextElement(&iter)) != NULL){
    Group* tmp = (Group*)elem;
    iter_g = createIterator(tmp->groups);

    //add rectangles from curr group
    while((elem_g = nextElement(&iter_g)) != NULL){
      Group* tmp = (Group*)elem_g;
      insertBack(addTo, tmp);
     }

    if(tmp->groups)
      digForGroups(tmp->groups, addTo, depth+1);
  }
}
void digForPaths(List* g, List* addTo){
  ListIterator iter = createIterator(g);
  void* elem;

  ListIterator iter_p;
  void* elem_p;

  while ((elem = nextElement(&iter)) != NULL){
    Group* tmp = (Group*)elem;
    iter_p = createIterator(tmp->paths);

    //add rectangles from curr group
    while((elem_p = nextElement(&iter_p)) != NULL){
      Path* tmp = (Path*)elem_p;
      insertBack(addTo, tmp);
    }

    if(tmp->groups)
      digForPaths(tmp->groups, addTo);
  }
}
// Function that returns a list of all circles in the image.
List* getCircles(SVGimage* img){
  List *toReturn = initializeList(&circleToString, &defectiveDelete, &compareCircles);
  if(img == NULL || img->circles == NULL)
    return toReturn;

  ListIterator iter = createIterator(img->circles);

  void* elem;
  //rects in img->rects
  while ((elem = nextElement(&iter)) != NULL){
    Circle* tmp = (Circle*)elem;
    insertBack(toReturn, tmp);
  }

  //add rects in groups
  digForCircles(img->groups, toReturn);
  return toReturn;
}
// Function that returns a list of all groups in the image.
List* getGroups(SVGimage* img){
  List *toReturn = initializeList(&groupToString, &defectiveDelete, &compareGroups);
  if(img == NULL || img->groups == NULL)
    return toReturn;

  ListIterator iter = createIterator(img->groups);

  void* elem;
  //groups in img->griyos
  while ((elem = nextElement(&iter)) != NULL){
    Path* tmp = (Path*)elem;
    insertBack(toReturn, tmp);
  }

  //add rects in groups
  digForGroups(img->groups, toReturn,1);
  return toReturn;
}
// Function that returns a list of all paths in the image.
List* getPaths(SVGimage* img){
  List *toReturn = initializeList(&pathToString, &defectiveDelete, &comparePaths);

  if(img == NULL || img->paths == NULL)
    return toReturn;

  ListIterator iter = createIterator(img->paths);
  void* elem;
  //groups in img->griyos
  while ((elem = nextElement(&iter)) != NULL){
    Path* tmp = (Path*)elem;
    insertBack(toReturn, tmp);
  }
  digForPaths(img->groups, toReturn);

  return toReturn;
}


/* For the four "num..." functions below, you need to search the SVG image for components  that match the search
  criterion.  You may wish to write some sort of a generic searcher fucntion that accepts an image, a predicate function,
  and a dummy search record as arguments.  We will discuss such search functions in class
 NOTE: For consistency, use the ceil() function to round the floats up to the nearest integer once you have computed
 the number you need.  See A1 Module 2 for details.
 *@pre SVGimgage exists, is not NULL, and has not been freed.  The search criterion is valid
 *@post SVGimgage has not been modified in any way
 *@return an int indicating how many objects matching the criterion are contained in the image
 *@param obj - a pointer to an SVG struct
 *@param 2nd - the second param depends on the function.  See details below
 */

// Function that returns the number of all rectangles with the specified area
int numRectsWithArea(SVGimage* img, float area){
  int ctr = 0;
  if(img == NULL)
    return 0;

  float areaR;
  List* allRects = getRects(img);
  ListIterator iter = createIterator(allRects);
  void* elem;
  //groups in img->griyos
  while ((elem = nextElement(&iter)) != NULL){
    Rectangle* tmp = (Rectangle*)elem;
    areaR = (tmp->width) * (tmp->height);
    if(ceil(area) == ceil(areaR))
      ctr++;
  }
  freeList(allRects);
  return ctr;
}
// Function that returns the number of all circles with the specified area
int numCirclesWithArea(SVGimage* img, float area){
  int ctr = 0;
  if(img == NULL)
    return 0;

  float areaC;
  List* allRects = getCircles(img);
  ListIterator iter = createIterator(allRects);
  void* elem;
  //groups in img->griyos
  while ((elem = nextElement(&iter)) != NULL){
    Circle* tmp = (Circle*)elem;
    areaC = (tmp->r) * (tmp->r)* 3.14159265358979;

    if(ceilf(area) == ceilf(areaC))
      ctr++;
  }
  freeList(allRects);
  return ctr;
}
// Function that returns the number of all paths with the specified data - i.e. Path.data field
int numPathsWithdata(SVGimage* img, char* data){
  int ctr = 0;
  if(img == NULL)
    return 0;

  char* dataP;
  List* allPaths = getPaths(img);
  ListIterator iter = createIterator(allPaths);
  void* elem;
  //groups in img->griyos
  while ((elem = nextElement(&iter)) != NULL){
    Path* tmp = (Path*)elem;
    dataP= tmp->data;
    if(strcmp(dataP, data) == 0)
      ctr++;
  }
  freeList(allPaths);
  return ctr;
}
int listLength(List* list){
  if(list == NULL){
    return 0;
  }
  ListIterator iter = createIterator(list);
  int num = 0;
  while (nextElement(&iter) != NULL){
    num++;
  }

  return num;
}
int groupLength(Group* g){
  int len = 0;
  len += listLength(g->rectangles);
  len += listLength(g->circles);
  len += listLength(g->paths);
  len += listLength(g->groups);

  printf("\ngrouplength = %d", len);
  return len;
}
// Function that returns the number of all groups with the specified length - see A1 Module 2 for details
int numGroupsWithLen(SVGimage* img, int len){
  int ctr = 0;
  if(img == NULL)
    return 0;

  List* allGroups = getGroups(img);
  ListIterator iter = createIterator(allGroups);
  void* elem;
  //groups in img->griyos
  while ((elem = nextElement(&iter)) != NULL){
    Group* tmp = (Group*)elem;

    if(len == groupLength(tmp))
      ctr++;
  }
  freeList(allGroups);
  return ctr;
}

/*  Function that returns the total number of Attribute structs in the SVGimage - i.e. the number of Attributes
    contained in all otherAttributes lists in the structs making up the SVGimage
    *@pre SVGimgage  exists, is not NULL, and has not been freed.
    *@post SVGimage has not been modified in any way
    *@return the total length of all attribute structs in the SVGimage
    *@param obj - a pointer to an SVG struct
*/
int numAttr(SVGimage* img){
  if(img == NULL)
    return 0;

  int ctr = 0;
  ctr += listLength(img->otherAttributes);
  List* allGroups = getGroups(img);
  List* allRects = getRects(img);
  List* allCircs = getCircles(img);
  List* allPaths = getPaths(img);

  //all attr from groups
  ListIterator iter = createIterator(allGroups);
  void* elem;
  while ((elem = nextElement(&iter)) != NULL){
    Group* tmp = (Group*)elem;

    ctr+=listLength(tmp->otherAttributes);
  }
  freeList(allGroups);

  //all attr from rects
  iter = createIterator(allRects);
  while ((elem = nextElement(&iter)) != NULL){
    Rectangle* tmp = (Rectangle*)elem;
    ctr += listLength(tmp->otherAttributes);
  }
  freeList(allRects);

  //all attr from circs
  iter = createIterator(allCircs);
  while ((elem = nextElement(&iter)) != NULL){
    Circle* tmp = (Circle*)elem;
    ctr += listLength(tmp->otherAttributes);
  }
  freeList(allCircs);

  //all attr from paths
  iter = createIterator(allPaths);
  while ((elem = nextElement(&iter)) != NULL){
    Path* tmp = (Path*)elem;
    ctr += listLength(tmp->otherAttributes);
  }
  freeList(allPaths);

  return ctr;
}
int makeEmpty(char* fname){
  SVGimage* toReturn = malloc(sizeof(SVGimage));

  strcpy(toReturn->namespace, "http://www.w3.org/2000/svg");
  strcpy(toReturn->title, "");
  strcpy(toReturn->description, "");
  initLists(toReturn);
  if(!validateSVGimage(toReturn, "parser/svg.xsd")){
      return 0;
  }
  int flag = writeSVGimage(toReturn, fname);
  deleteSVGimage(toReturn);
  return flag;
}