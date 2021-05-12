#pragma once

#include "core.h"

#include <vector>
#include <memory>


using std::vector;
using std::unique_ptr;
using std::shared_ptr;


struct ABSTRACT_BASE RtNode_Base {

};


struct RtFlow {
	
};


struct RtNode_Branch {
	bool parameter;
	bool signal;

};


struct RtNode_Func {
	unordered_map<ref_ptr<Node_Base>, unique_ptr<RtNode_Base>> node_map;

};