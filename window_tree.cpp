#include "window_tree.hpp"

extern "C"{
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
}

//#include <glog/logging.h>
#include <cstring>
#include <algorithm>

using ::std::unique_ptr;
using ::std::string;
using ::std::max;

windowtree::windowtree(){
  root=NULL;
}

void windowtree::destroytree(node* leaf){
  if (leaf!=NULL){
    destroytree(leaf->left);
    destroytree(leaf->right);
    delete leaf;
  }
}

windowtree::~windowtree(){
  destroytree(root);
}


void windowtree::insert(Window* newwin, node* n){ // adds new window to end of tree
  if (n->t == full){
    n->left = n;
    n->right = new node;
    n->right->t = full;
    n->t = horizontal;
    
    calcSizes(n->X, n->Y, n->width, n->height, n);
  }
  else insert(newwin, n->right);
}

bool windowtree::removeWindow(Window* w, node* n){ // search and remove by window
  if (n->t == full){ // ...then root is leaf, check for w
    if (w == n->w){ // ...then root points to w, remove root
      node* sibling; 
      if (n->parent->left == n){ 
        sibling = n->parent->right;
      }
      else{
        sibling = n->parent->left;
      }

      // 1) Reconfigure tree to retain balance
      reparentToGrandparent(sibling);

      // 2) Destroy n and it's parent
      destroytree(n->parent);

      // 3) Recalculate sizes of modified subtree
      calcSizes(sibling->parent->X, sibling->parent->Y, sibling->parent->width, sibling->parent->height, sibling->parent);

      return true;
    }
    else return false; // path ended, window not found
  }

  return removeWindow(w, n->left) || removeWindow(w, n->right); // otherwise, dfs into n's subtrees
}

bool windowtree::removeContainer(node* n, node* root){ // search and remove by node
  if (n == root){
    // 1) Reparent Sibling
    if (n == n->parent->right) reparentToGrandparent(n->parent->left); 
    else reparentToGrandparent(n->parent->right);

    // 2) Remove subtree
    destroytree(n); // remove
    
    // 3) Recalculate sizes
    calcSizes(n->parent->X, n->parent->Y, n->parent->width, n->parent->height, n->parent);
    
    return true;
  }
  else if (root->t != full){
    return removeContainer(n, root->left) || removeContainer(n, root->right);
  }
  else return false;
}

void windowtree::reparentToGrandparent(node* n){
  // 1) point new parent to node
  if (n->parent->parent->left == n->parent) n->parent->parent->left = n;
  else n->parent->parent->right = n; 

  // 2) point node to new parent
  n->parent = n->parent->parent;
}

void windowtree::calcSizes(int X, int Y, int width, int height, node* root){ // Calculates absolute size of each tiled window
  // set window attributes
  root->X = X;
  root->Y = Y;
  root->width = width;
  root->height = height;

  // Root divided evenly between subcontainers
  if (root->t != full){ // if has subcontainers
    int X2 = X; // coords for 2nd subcontainer
    int Y2 = Y; //

    if (root->t == horizontal){ // horizontally divide space
      height = height/2;
      X2 = X2 + height;
    }
    else{ // vertically divide space
      width = width/2;
      Y2 = Y2 + width;
    }

    // recurse depth-first through tree
    calcSizes(X, Y, width, height, root->left);
    calcSizes(X2, Y2, width, height, root->right);
  }
}

void windowtree::switchMode(node* n, enum nodetype t){ // Changes how subtree is tiled
  if (n->t != t){ // skip if no change

    // ensures container with two subcontainers always divides space
    if (t == full &&
    n->left != NULL &&
    n->right != NULL){
      //LOG(ERROR) << "Window can only span container if it is the only child."; 
      return;
    }

    // assign new mode and recalculate sizes for modified subtree
    n->t = t;
    calcSizes(n->X, n->Y, n->width, n->height, n);
  }
}