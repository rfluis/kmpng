#include <map>
#include <vector>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>
#include <zlib.h>
#include <iostream>

typedef std::pair<const std::vector<unsigned char>*,unsigned int> stringcursor;

struct image{
  unsigned int x;
  unsigned int y;
  unsigned int *p;
  image (unsigned int xx,unsigned int yy):x(xx),y(yy){
    p=(unsigned int*) malloc(x*y*4);
    return;
    }
  image(const image& rhs){
    x=rhs.x;
    y=rhs.y;
    p=(unsigned int*) malloc(x*y*4);
    memcpy(p,rhs.p,x*y*4);
    return;
    }
  image& operator=(image rhs){
    unsigned int xx,yy;
    unsigned int *pp;
    xx = rhs.x;
    yy = rhs.y;
    pp = rhs.p;
    rhs.x = x;
    rhs.y = y;
    rhs.p = p;
    x = xx;
    y = yy;
    p = pp;
    return *this; // Copy & swap idiom + return self-ref for chaining  
    }
  ~image(){free(p);}
  };
  
static void PngWriteCallback(png_structp  png_ptr, png_bytep data, png_size_t length) {
    std::vector<unsigned char> *dstp = reinterpret_cast<std::vector<unsigned char>*>(png_get_io_ptr(png_ptr));
    dstp->insert(dstp->end(),(unsigned char*)data,(unsigned char*)data+length);
    return;
}
static void PngReadCallback(png_structp  png_ptr, png_bytep data, png_size_t length) {
    stringcursor *srcp = reinterpret_cast<stringcursor*>(png_get_io_ptr(png_ptr));
    const std::vector<unsigned char>& src = *(srcp->first);
    unsigned int& cursor = srcp->second;
    memcpy(data,&src[cursor],length);
    cursor+=length;
    return;
}

image decode_png(const std::vector<unsigned char>& src){
  stringcursor in;
  in.first = &src;
  in.second = 0;
  png_byte color_type;
  png_byte bit_depth;
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png_create_info_struct(png);
  setjmp(png_jmpbuf(png));
  png_set_read_fn(png, &in, PngReadCallback);
  png_read_info(png, info);
  unsigned int sx = png_get_image_width(png, info);
  unsigned int sy = png_get_image_height(png, info);
  image dst(sx,sy);
  color_type = png_get_color_type(png, info);
  bit_depth  = png_get_bit_depth(png, info);
  if(bit_depth == 16) png_set_strip_16(png);
  if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
  if(png_get_valid(png, info, PNG_INFO_tRNS))  png_set_tRNS_to_alpha(png);
  if(color_type == PNG_COLOR_TYPE_RGB ||color_type == PNG_COLOR_TYPE_GRAY ||color_type == PNG_COLOR_TYPE_PALETTE) png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
  if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
  png_read_update_info(png, info);
  unsigned char** rows = (unsigned char**) malloc (sizeof(unsigned char*)*sy);
  for (size_t y = 0; y < sy; ++y) rows[y] = ((unsigned char*) dst.p) + y*sx*4;
  png_read_image(png,rows);
  png_destroy_read_struct(&png, &info,(png_infopp)NULL);
  free(rows);
  return dst;
  }
  
int encode_png(std::vector<unsigned char>& dst,const image& src){
  unsigned int sx = src.x;
  unsigned int sy = src.y;
  unsigned int *imgptr = src.p;
  png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info_ptr = png_create_info_struct(p);
  setjmp(png_jmpbuf(p));
  png_set_IHDR(p, info_ptr, sx, sy, 8,PNG_COLOR_TYPE_RGBA,PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
  png_set_compression_level(p, 9);
  unsigned char** rows = (unsigned char**) malloc (sizeof(unsigned char*)*sy);
  for (size_t y = 0; y < sy; ++y) rows[y] = ((unsigned char*) imgptr) + y*sx*4;
  png_set_rows(p, info_ptr,(unsigned char**) &rows[0]);
  dst = std::vector<unsigned char>();
  png_set_write_fn(p,&dst, PngWriteCallback, NULL);
  png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
  if (p) png_destroy_write_struct(&p, NULL);
  free(rows);
  return 0;
  }

struct point{
  float x,y,z;  
  point(){x=y=z=0; return;}
  point(float xx,float yy,float zz):x(xx),y(yy),z(zz){return;}
  point& operator+=(const point& rhs){
    x+=rhs.x;
    y+=rhs.y;
    z+=rhs.z;
    return *this;
    }
  point& operator-=(const point& rhs){
    x-=rhs.x;
    y-=rhs.y;
    z-=rhs.z;
    return *this;
    }
  point operator+(const point& rhs)const{
    return point(x+rhs.x,y+rhs.y,z+rhs.z);
    }
  point operator-(const point& rhs)const{
    return point(x-rhs.x,y-rhs.y,z-rhs.z);
    }
  float operator*(const point& rhs){ return (x*rhs.x+y*rhs.y+z*rhs.z);}
  float norm2(){return (x*x+y*y+z*z);}
  bool operator<(const point& rhs)const{
    if (x<rhs.x) return true;
    if (x>rhs.x) return false;
    if (y<rhs.y) return true;
    if (y>rhs.y) return false;
    if (z<rhs.z) return true;
    if (z>rhs.z) return false;
    return false;
  }
  bool operator> (const point& rhs)const{ return rhs < *this; }
  bool operator<=(const point& rhs)const{ return !(*this > rhs); }
  bool operator>=(const point& rhs)const{ return !(*this < rhs); }
  bool operator ==(const point& rhs)const{
    if (x!=rhs.x) return false;
    if (y!=rhs.y) return false;
    if (z!=rhs.z) return false;
    return true;
    }
};

struct px{unsigned char r,g,b,a;};
  
point uint_to_fpoint(unsigned int p){
  const px& pix = reinterpret_cast<const px&>(p);
  return point(static_cast<float>(pix.r),static_cast<float>(pix.g),static_cast<float>(pix.b));
  }

unsigned int fpoint_to_uint(point p){
  unsigned int up;
  px& pix = reinterpret_cast<px&>(up);
  pix.r = p.x;
  pix.g = p.y;
  pix.b = p.z;
  pix.a = 255;
  return up;
  }

struct kmp{
  point p;
  float w; // weight
  kmp(){ p = point(); w=0;}
  kmp(point pp,float ww):p(pp),w(ww){return;}
  void normalize(){
    p.x/=w;
    p.y/=w;
    p.z/=w;
    return;
    }
  point scale()const{return point(p.x*w,p.y*w,p.z*w);}
  bool operator <(const kmp& rhs)const{
    if (p<rhs.p) return true;
    if (p>rhs.p) return false;
    if (w<rhs.w) return true;
    if (w>rhs.w) return false;
    }
  bool operator ==(const kmp& rhs)const{
    if (p==rhs.p && w==rhs.w) return true;
    return false;
    }
  };
  


/* k-means
 * 1. Assign a centroid to each point (find closest) for each point for each centroid take closest
 * 2. Recalculate centroids (Loop over points and sum , then do average with weights)
 * 3. If new centroids are not the same go to 1.
 * */

std::vector<unsigned int> km_pick_centroids(const std::vector<kmp>& centroid,const std::vector<kmp>& points){
  unsigned int kmpoints = points.size();
  unsigned int ncntrds = centroid.size();
  std::vector<unsigned int> ca(kmpoints);
  
  for (unsigned int idx=0;idx<kmpoints;idx++){ // idx -> points
      float dist,ndist; // distances
      unsigned int c;
      point delta; // difference between a point and a centroid
      c = 0;
      delta = centroid[0].p - points[idx].p;
      dist = delta.norm2();
      for (unsigned int idy=1;idy<ncntrds;idy++){ // idy -> centroids
        delta = centroid[idy].p - points[idx].p;
        ndist = delta.norm2();
        if (ndist<dist) { c = idy; dist = ndist;}
        }
      ca[idx] = c;
    }
  
  return ca;
  }

float kmerror(const std::vector<kmp>& centroid,const std::vector<kmp>& points){
  unsigned int kmpoints = points.size();
  unsigned int ncntrds = centroid.size();
  std::vector<unsigned int> ca(kmpoints); // Centroid assignment ca[idx] = centroid of points[idx]
  float error=0;
  
  ca = km_pick_centroids(centroid,points);
    
  for (unsigned int idx=0;idx<kmpoints;idx++){
    point delta;
    delta = centroid[ca[idx]].p - points[idx].p;
    error += delta.norm2()*points[idx].w;
    }
  return error;
  }


std::vector<kmp> kmeans(const std::vector<kmp>& points,unsigned int ncntrds){
  unsigned int kmpoints = points.size();
  std::vector<kmp> centroid(ncntrds);
  std::vector<kmp> newcentroid(ncntrds); // check if ncntrds>=kmpoints before call
  std::vector<unsigned int> ca(kmpoints); // Centroid assignment ca[idx] = centroid of points[idx]
  bool fi = true; // fisrt iteration flag
  for (unsigned int idx=0;idx<ncntrds;idx++) newcentroid[idx]=points[idx]; // random centroid assignment
  //unsigned int counter=0;
  while (fi || centroid!=newcentroid){ // make first iteration and more while old!=new
    //std::cout << "[" << counter << "]";
    //counter++;
    centroid = newcentroid;
    /*update ca : give to each point a centroid */
    ca = km_pick_centroids(centroid,points);
    /*update centroids */
    for (unsigned int idx=0;idx<ncntrds;idx++) newcentroid[idx] = kmp(); // clear centroids
    for (unsigned int idx=0;idx<kmpoints;idx++) {newcentroid[ca[idx]].p+=points[idx].scale(); newcentroid[ca[idx]].w+=points[idx].w;}
    for (unsigned int idx=0;idx<ncntrds;idx++) if (newcentroid[idx].w > 1) newcentroid[idx].normalize();
    fi = false;
    
    }
  return centroid;
  }
  
std::vector<kmp> img_histogram(const image& src){
  unsigned int size = src.x*src.y;
  std::map<point,unsigned int> hist;
  for (unsigned int idx=0;idx<size;idx++) hist[uint_to_fpoint(src.p[idx])]++;
  std::vector<kmp> points;
  for (auto itr=hist.begin();itr!=hist.end();itr++) points.push_back(kmp(itr->first,static_cast<float>(itr->second)));
  return points;
  }

image kmimg (const image& src){
  image ans(src.x,src.y);
  unsigned int size = src.x*src.y;
  std::vector<kmp> points = img_histogram(src);
  /*
  for (unsigned int idx=0;idx<points.size();idx++) {
    std::cout << "px:" << idx << " - " ;
    std::cout << points[idx].p.x << " " ;
    std::cout << points[idx].p.y << " " ;
    std::cout << points[idx].p.z << " " ;
    std::cout << points[idx].w << std::endl;
  }
  */
  unsigned int kmpoints = points.size();
  unsigned int maxc = kmpoints > 256 ? 256 : kmpoints-1;
  std::vector<float> merror(maxc); // errors (correct index)
  std::vector<std::vector<kmp>> mcentroid(maxc); // centroids for each number of centroids
  
  for (unsigned int c=2;c<=maxc;c++) {
    float error;
    std::cout << "Trying c=" << c << "/" << maxc << std::flush;
    std::vector<kmp> centroid;
    centroid = kmeans(points,c);
    error = kmerror(centroid,points);
    std::cout << " had error=" << error << std::endl;
    //for (unsigned int idx=0;idx<centroid.size();idx++) std::cout << "idx:" << idx << " - " << centroid[idx].p.x << " " << centroid[idx].p.y << " " << centroid[idx].p.z << " " << centroid[idx].w << std::endl; 
    merror[c-2] = error;
    mcentroid[c-2] = centroid;
    }
  float minerror = merror[0];
  unsigned int optimc = 2;
  for (unsigned int c=2;c<=maxc;c++) if (merror[c-2]<minerror) {minerror = merror[c-2]; optimc=c;}
  std::vector<kmp> centroid = mcentroid[optimc-2];
  std::vector<unsigned int> ca(kmpoints);
  ca = km_pick_centroids(centroid,points);
  std::cout << "Number of colours = " << optimc << std::endl;
  std::vector<unsigned int> pallete(centroid.size());
  for (unsigned int idx=0;idx<pallete.size();idx++) pallete[idx]=fpoint_to_uint(centroid[idx].p);
  std::map<point,unsigned int> translat;
  for (unsigned int idx=0;idx<kmpoints;idx++) translat[points[idx].p] = pallete[ca[idx]];
  for (unsigned int idx=0;idx<size;idx++) ans.p[idx] = translat[uint_to_fpoint(src.p[idx])];
  return ans;
  }

void load_file(std::vector<unsigned char>& buffer, const std::string& filename){
  FILE *file;
  if ((file = fopen(filename.c_str(), "r+"))){
    size_t size;
    fseek(file,0,SEEK_END);
    size = ftell(file);
    fseek(file,0,SEEK_SET);
    buffer.resize(size);
    fread(&buffer[0],size,1,file);
    fclose(file);return;
    }
  return;
}

void save_file(const std::string& filename,const std::vector<unsigned char>& buffer){
  FILE *file;
  if ((file = fopen(filename.c_str(), "w+")))
  {
    fwrite(&buffer[0],buffer.size(),1,file);
    fclose(file);
    return;
  }
  return;
}  

int main(int argc,char **argv){
  if (argc==2) {
    std::vector<unsigned char> buffer;
    load_file(buffer,argv[1]);
    image src = decode_png(buffer);
    image dst = kmimg(src);
    encode_png(buffer,dst);
    save_file("km_"+std::string(argv[1]),buffer);
    return 0;
    }
  
  if (argc==3) {
    std::vector<unsigned char> buffer;
    load_file(buffer,argv[1]);
    image src = decode_png(buffer);
    image dst = kmimg(src);
    encode_png(buffer,dst);
    save_file(argv[2],buffer);
    return 0;
    }
    return 0;
  }
