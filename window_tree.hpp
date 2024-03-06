/* 
  window_tree
  Jonathan Hackett

  Tree implementation for flotise's window tiling

    - each floating set of tiles has one associated binary tree
    - each node represents a window container
    - each subtree represents a subcontainer within the root container
    - containers can either display a single window or the contents of two subcontainers
    - space within containers is shared evenly between subcontainers - split horizontally or vertically

  NOTE: In a normalized window tree, each container either has two subcontainers or a single window.
  Thus, the window tree must be balanced - i.e. each node has a depth of 0.
  This is done to prevent oversized trees.
*/
extern "C"{
#include <X11/Xlib.h>
}
#include <unordered_map>
#include <memory>

enum nodetype {
  horizontal, // split across X axis
  veritcal,  // split across Y axis
  full // span entire container (i.e. container with one window)
};

struct node{
  int key;

  // container attributes
  int X, Y; //location relative to parent
  int width, height; //absolute size on screen
  Window w; //ID of directly contained window

  // pointers to neighbours
  node *left;
  node *right;
  node *parent;

  // defines window display mode
  enum nodetype t;
};

class windowtree{
  public:
    windowtree();
    ~windowtree();

    void insert(const Window newwin);
    bool removeWindow(Window w, node *n);
    bool removeContainer(node *n, node *root);
    node *search(Window w);
    void destroytree();
    void calcSizes(int X, int Y, int width, int height, node* root);
    void switchMode(node* n, enum nodetype t);
    node *root;
  private:
    void destroytree(node *root);
    node *search(Window *w, node *root);
    void reparentToGrandparent(node* n);
    
    //::std::unordered_map<Window, node*> leaves_; 

};