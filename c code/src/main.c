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
  SVGimage* svg = createValidSVGimage(argv[1], argv[2]);

  if(svg){
    writeSVGimage(svg, "dst.svg");
    deleteSVGimage(svg);
     }
  return 0;
}
