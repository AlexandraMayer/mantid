#include "GLActor.h"


GLActor::~GLActor()
{
}

bool GLActor::accept(const GLActorVisitor& visitor)
{
  return visitor.visit(this);
}

GLColor GLActor::makePickColor(size_t pickID)
{
    pickID += 1;
    unsigned char r,g,b;
    r=(unsigned char)(pickID/65536);
    g=(unsigned char)((pickID%65536)/256);
    b=(unsigned char)((pickID%65536)%256);
    return GLColor(r,g,b);
}

size_t GLActor::decodePickColor(const GLColor& c)
{
  unsigned char r,g,b;
  c.get(r,g,b);
  return decodePickColor(r,g,b);
}

size_t GLActor::decodePickColor(unsigned char r,unsigned char g,unsigned char b)
{
  unsigned int index = r;
  index *= 256;
  index += g;
  index *= 256;
  index += b - 1;
  //std::cerr << "decode " << int(r)<<' '<<int(g)<<' '<<int(b)<<' '<<index<<std::endl;
  return index;
}

GLColor GLActor::defaultDetectorColor()
{
  return GLColor(255,100,0);
}
