#include "window_tree.hpp"

extern "C"{
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
}


windowtree::windowtree(){
  root=NULL;
}

windowtree::~windowtree(){
  destroytree();
}

void windowtree::destroytree(node *leaf){
  if (leaf!=NULL){
    destroytree(leaf->left);
    destroytree(leaf->right);
    delete leaf;
  }
}

void windowtree::insert(Window *newwin, node *leaf){
  if(leaf->left!=NULL){
    if(leaf->right!=NULL){
      insert(newwin, leaf->left)
    }
    else{
      leaf->right=new node;
      leaf->right->w = leaf->w;
      leaf->right->pred = leaf;
      leaf->w = NULL;
      leaf->right->left= new node;
      leaf->right->left->w = newwin;
      leaf->right->left->pred = leaf->right;
      leaf->right->left->left = NULL;
      leaf->right->left->right = NULL;
    }
  }
  else{
    leaf->left = new node;
    leaf->left->w = leaf->w;
  }
}
