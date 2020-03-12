#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "LinkedListAPI.h"
#include "SVGParser.h"

char* cNull(void* lol){
  return NULL;
}
int iNull(const void* x , const void* y){
  return 1;
}
void vNull(){
  return;
}
int main(int argc, char **argv)
{
  makeEmpty("test.svg");
  return 0;
}
