
#include <fstream>
#include "inou_cfg.hpp"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stack>
#include "lgraphbase.hpp"
#include "lgedgeiter.hpp"
using std::string;
using std::vector;
using std::map;
Inou_cfg_options_pack::Inou_cfg_options_pack() {

	Options::get_desc()->add_options()
					("cfg_output,o", boost::program_options::value(&cfg_output), "cfg output <filename> for graph")
					("cfg_input,i", boost::program_options::value(&cfg_input), "cfg input <filename> for graph");

	boost::program_options::variables_map vm;
	boost::program_options::store(
					boost::program_options::command_line_parser(Options::get_cargc(), Options::get_cargv()).options(
									*Options::get_desc()).allow_unregistered().run(), vm);
	if (vm.count("cfg_output")) {
		cfg_output = vm["cfg_output"].as<string>();
	}
	else {
		cfg_output = "output.cfg";
	}

	if (vm.count("cfg_input")) {
		cfg_input = vm["cfg_input"].as<string>();
	}
	else {
		cfg_input = "test.cfg";
	}

	console->info("inou_cfg cfg_output:{} cfg_input:{} graph_name:{}", cfg_output, cfg_input, graph_name);
}

Inou_cfg::Inou_cfg() {
}

Inou_cfg::~Inou_cfg() {
}

vector<LGraph *> Inou_cfg::generate() {

	vector<LGraph *> lgs;
	if (opack.graph_name != "") {
		lgs.push_back(new LGraph(opack.lgdb_path, opack.graph_name, false)); // Do not clear
		// No need to sync because it is a reload. Already sync
	}
	else {
		assert(opack.cfg_input != "");

		lgs.push_back(new LGraph(opack.lgdb_path));
		string cfg_file = opack.cfg_input;

    int fd = open(cfg_file.c_str(), O_RDONLY);
		if (fd<0) {
      console->error("cannot find input file {}\n", cfg_file);
			exit(-3);
		}

	  struct stat sb;
	  fstat(fd, &sb);
	  //printf("Size: %lu\n", (uint64_t)sb.st_size);

	  char* memblock = (char* )mmap(nullptr, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
	  if (memblock == MAP_FAILED) {
      console->error("error, mmap failed\n");
	  	exit(-3);
	  }

    //cfg_2_lgraph_bk(&memblock, lgs[0]);
    cfg_2_lgraph(&memblock,lgs);;

		for(int i = 0; i< lgs.size(); i++){
			lgs[i]->sync();
		}
    close(fd);
	}
	return lgs;
}


void Inou_cfg::cfg_2_lgraph(char** memblock, vector<LGraph*>& lgs){
	string s;
	vector<map<string, Index_ID>>       name2id_gs(1);   //gs = graphs
	vector<map<string, vector<string>>> chain_stks_gs(1);//chain_stacks for every graph, use vector to implement stack
	vector<string>                      nname_bg_gs(1);   //nname = node name
	vector<Index_ID >                   nid_ed_gs(1);
  map<string, uint32_t >              nfirst2gid;//map for every sub-graph and its first node name
	LGraph* gtop = lgs[0];

	bool gtop_bg_nname_recorded = false;

	char *p = strtok(*memblock, "\n\r\f");

	while(p){
		vector <string> words = split(p);

		if(*(words.begin()  ) == "END")
			break;

		string w1st = *(words.begin()  );
    string w3rd = *(words.begin()+2);
		string w6th = *(words.begin()+5);

		uint32_t gsub_id = std::stoi(w3rd);

		if(!gtop_bg_nname_recorded){
			nname_bg_gs[0] = w1st;
			gtop_bg_nname_recorded = true;
		}

		string dfg_data = p;

		if(w3rd != "0" && std::stoi(w3rd)>= lgs.size()) {//create sub-graph if different scope id
			lgs.push_back(new LGraph(opack.lgdb_path));
			fmt::print("lgs size:{}", lgs.size());
			name2id_gs.resize(name2id_gs.size()+1);
			chain_stks_gs.resize(chain_stks_gs.size()+1);
			nid_ed_gs.resize(nid_ed_gs.size()+1);
			nname_bg_gs.push_back(w1st);
			nfirst2gid[w1st] = gsub_id;
			build_graph(words, dfg_data, lgs[gsub_id], nfirst2gid, name2id_gs[gsub_id], chain_stks_gs[gsub_id], nid_ed_gs[gsub_id]);
		}
		else if(w3rd != "0")//construct sub-graph for function definition
			build_graph(words, dfg_data, lgs[gsub_id], nfirst2gid, name2id_gs[gsub_id], chain_stks_gs[gsub_id], nid_ed_gs[gsub_id]);
		else //build top graph
			build_graph(words, dfg_data, gtop, nfirst2gid, name2id_gs[0], chain_stks_gs[0], nid_ed_gs[0]);




		fmt::print("\n");
		p = strtok(nullptr, "\n\r\f");
	}//end while loop

	/*
    deal with GIO for every graph
  */

	for(int i = 0; i<lgs.size();i++){
		//Graph input
		Node gio_node_bg = lgs[i]->create_node();
		fmt::print("create node:{}, nid:{}\n", "GIO", gio_node_bg.get_nid());
		lgs[i]->node_type_set(gio_node_bg.get_nid(), GraphIO_Op);
		Index_ID src_nid = gio_node_bg.get_nid();
		Index_ID dst_nid = name2id_gs[i][nname_bg_gs[i]];
		lgs[i]->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));

		//Graph output
		Node gio_node_ed = lgs[i]->create_node();
		fmt::print("create node:{}, nid:{}\n", "GIO", gio_node_ed.get_nid());
		lgs[i]->node_type_set(gio_node_ed.get_nid(), GraphIO_Op);
		src_nid = nid_ed_gs[i];
		fmt::print("total node number:{}\n", name2id_gs[0].size());
		dst_nid = gio_node_ed.get_nid();;
		lgs[i]->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
	}



	for(int i =0; i < chain_stks_gs.size(); i++){
		for (auto& x : chain_stks_gs[i]){
			fmt::print("\ncurrent is chain_stks_gs[{}], vid {} content is\n", i, x.first);
			for (auto j = x.second.rbegin(); j != x.second.rend(); ++j)
				fmt::print("{}\n",*j);
		}
	}


}

void Inou_cfg::build_graph(vector<string>& words, string& dfg_data, LGraph* g, map<string,uint32_t >& nfirst2gid,
													 map<string, Index_ID>& name2id, map<string, vector<string>>& chain_stks, int64_t& nid_end) {

	fmt::print("dfg_data:{}\n", dfg_data);
	string w1st = *(words.begin()  );
	string w2nd = *(words.begin()+1);
	string w3rd = *(words.begin()+2);
	string w4th = *(words.begin()+3);
	string w5th = *(words.begin()+4);
	string w6th = *(words.begin()+5);
	string w7th = *(words.begin()+6);
	string w8th = *(words.begin()+7);
	string w9th;
	string w10th;
	if(w6th == "if" || w6th == "::{")
		w9th = *(words.begin()+8);

	if(w6th == "if")
		w10th = *(words.begin()+9);

	/*
    I.process 1st node
    only assign node type for first K in every line of cfg
  */
	if(name2id.count(w1st) == 0){//if node has not been created before
		Node new_node = g->create_node();
		name2id[w1st] = new_node.get_nid();
		nid_end =  new_node.get_nid();//keep update the latest final nid

		fmt::print("create node:{}, nid:{}\n", w1st, name2id[w1st]);

		g->set_node_wirename(new_node.get_nid(), dfg_data.c_str());
    g->node_loc_set(new_node.get_nid(),opack.cfg_input.c_str(), (uint32_t)std::stoi(w4th), (uint32_t)std::stoi(w5th));

		if(     w6th == ".()")
			g->node_type_set(name2id[w1st], CfgFunctionCall_Op);
		else if(w6th == "for")
			g->node_type_set(name2id[w1st], CfgFor_Op);
		else if(w6th == "while")
			g->node_type_set(name2id[w1st], CfgWhile_Op);
		else if(w6th == "if")
			g->node_type_set(name2id[w1st], CfgIf_Op);
		else
			g->node_type_set(name2id[w1st], CfgAssign_Op);
	}
	else{
		g->set_node_wirename(name2id[w1st], dfg_data.c_str());
		g->node_loc_set(name2id[w1st], opack.cfg_input.c_str(), (uint32_t)std::stoi(w4th), (uint32_t)std::stoi(w5th));

		if(     w6th == ".()")
			g->node_type_set(name2id[w1st], CfgFunctionCall_Op);
		else if(w6th == "for")
			g->node_type_set(name2id[w1st], CfgFor_Op);
		else if(w6th == "while")
			g->node_type_set(name2id[w1st], CfgWhile_Op);
		else if(w6th == "if")
			g->node_type_set(name2id[w1st], CfgIf_Op);
		else if(w6th == "::{"){
			g->node_subgraph_set(name2id[w1st],nfirst2gid[w9th]);//use nfirst2gid to get sub-graph gid
		}
		else{
			g->node_type_set(name2id[w1st], CfgAssign_Op);
		}
	}


	/*
    II-0.process 2nd node and 9th node(if-else merging node)
  */

	if(w6th == "if" && name2id.count(w10th) == 0){//if node has not been created before
		Node new_node = g->create_node();
		name2id[w10th] = new_node.get_nid();
		nid_end =  new_node.get_nid();//keep update the latest final nid
		fmt::print("create node:{}, nid:{}\n", w10th, name2id[w10th]);

		g->node_type_set(name2id[w10th], CfgIfMerge_Op);
	}

	if(w2nd != "null" && name2id.count(w2nd) == 0){
		Node new_node = g->create_node();
		name2id[w2nd] = new_node.get_nid();
		nid_end =  new_node.get_nid();//keep update the latest final nid
		fmt::print("create node:{}, nid:{}\n", w2nd, name2id[w2nd]);
	}

	/*
    III.deal with edge connection
  */
	Index_ID src_nid, dst_nid;

	if(w6th == "if"){//special case: if
		//III-1. connect if node to the begin of "true chunk" statement
		src_nid = name2id[w1st];
		dst_nid = name2id[w8th];
		g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
		fmt::print("if statement, connect src_node {} to dst_node {} ----- 1\n", src_nid, dst_nid);

		//III-2. connect if node to the begin of "false chunk" statement
		if(w9th != "null"){
			src_nid = name2id[w1st];
			dst_nid = name2id[w9th];
			g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
			fmt::print("if statement, connect src_node {} to dst_node {} ----- 2\n", src_nid, dst_nid);
		}

		//III-3. connect top of stack to end if node
		if(chain_stks[w8th].back() != w10th){
			src_nid = name2id[chain_stks[w8th].back()];
      fmt::print("chain_stks[w8th] back{}\n", chain_stks[w8th].back());
			dst_nid = name2id[w10th];
			g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
			fmt::print("if statement, connect src_node {} to dst_node {} ----- 3\n", src_nid, dst_nid);
		}

		if(w9th != "null"){
			if(chain_stks[w9th].back() != w10th ){
				src_nid = name2id[chain_stks[w9th].back()];
				fmt::print("chain_stks[w9th] back{}\n", chain_stks[w9th].back());
				dst_nid = name2id[w10th];
				g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
				fmt::print("if statement, connect src_node {} to dst_node {} ----- 4\n", src_nid, dst_nid);
			}
		}
		else{//only one branch of if. EX.  K13  K30   63  169  if   tmp3  K15  null 'K13
			src_nid = name2id[w1st];
			dst_nid = name2id[w10th];
			g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
			fmt::print("if statement, connect src_node {} to dst_node {} ----- 5\n", src_nid, dst_nid);
		}


		//III-4. if it is an outer if statement, link w10th to w2nd
		if(w2nd != "null"){
			src_nid = name2id[w10th];
			dst_nid = name2id[w2nd];
			g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
			fmt::print("if statement, connect src_node {} to dst_node {} ----- 6\n", src_nid, dst_nid);
		}

		//III-5. figure out which stack w1st is belong to and push w10th into that stack
		for (auto const& x : chain_stks){
			if(w1st == x.second.back()){
				chain_stks[x.second[0]].push_back(w10th);
				break;
			}
		}

	}//end special case: if

	else if(w6th == "for"){
		//I. True: connect for node to body
		src_nid = name2id[w1st];
		dst_nid = name2id[w8th];
		g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
		fmt::print("for statement, connect src_node {} to dst_node {}\n", src_nid, dst_nid);
		//II. False: connect for node to next event
		src_nid = name2id[w1st];
		dst_nid = name2id[w2nd];
		g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
		fmt::print("for statement, connect src_node {} to dst_node {}\n", src_nid, dst_nid);
	}//end special case: for

	else if(w6th == "while"){
		//I. connect while node to body
		src_nid = name2id[w1st];
		dst_nid = name2id[w8th];
		g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
		fmt::print("while statement, connect src_node {} to dst_node {}\n", src_nid, dst_nid);

		//II. False: connect while node to next event
		src_nid = name2id[w1st];
		dst_nid = name2id[w2nd];
		g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
		fmt::print("while statement, connect src_node {} to dst_node {}\n", src_nid, dst_nid);
	}//end special case: while


	else if(w6th == "::{"){
		//connect to the begin of function call
		src_nid = name2id[w1st];
		dst_nid = name2id[w9th];
		g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
		fmt::print("function call statement, connect src_node {} to dst_node {} ----- 1\n", src_nid, dst_nid);
	}//end special case: ::{

	else if(w2nd == "null"){ //no w2nd to create edge, only update chain_stks
		bool belong_tops = false;
		string target_vec_id = w1st;

		//check equivalence between src_nid and every top of chain_stks
		for (auto const& x : chain_stks){
			if(w1st == x.second.back()){
				belong_tops = true;
				target_vec_id = x.first;
				break;
			}
		}
		if(!belong_tops)
			chain_stks[target_vec_id].push_back(w1st);

	}
	else if(w2nd != "null"){ //normal edge connection: Kx->Ky, update chain_stks
		src_nid = name2id[w1st];
		dst_nid = name2id[w2nd];
		g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
		fmt::print("normal case connection, connect src_node {} to dst_node {} ----- 5\n", src_nid, dst_nid);

		bool belong_tops = false;
		string target_vec_id = w1st;

		//check equivalence between src_nid and every top of chain_stks
		for (auto const& x : chain_stks){
			if(w1st == x.second.back()){
				belong_tops = true;
				target_vec_id = x.first;
				break;
			}
		}

		if(!belong_tops){
			chain_stks[target_vec_id].push_back(w1st);
			chain_stks[target_vec_id].push_back(w2nd);
		}
		else
			chain_stks[target_vec_id].push_back(w2nd);

	}

}//end of build_graph


/*
 * create lgraph based on every first two nodes defined in CFG table
 */


vector<string> Inou_cfg::split(const string& str){
	typedef string::const_iterator iter;
	vector<string> ret;

	iter i = str.begin();
	while(i != str.end()){
		i = find_if(i, str.end(), not_space);
    // find end of next word
		iter j = find_if(i, str.end(), space);

		//copy the characters in [i,j)
		if (i != str.end())
			ret.push_back(string(i,j));

		i = j;
	}
	return ret;
}

void Inou_cfg::lgraph_2_cfg(const LGraph* g, const string& filename ) {
  int line_cnt = 0;
  for (auto &idx : g->fast()) {
    if (g->get_node_wirename(idx) != nullptr) {
      fmt::print("{}\n",g->get_node_wirename(idx));//for now, just print out cfg, maybe mmap write later
      ++line_cnt;
    }
	}
  fmt::print("END\n");
  ++line_cnt;
  fmt::print("line_cnt = {}\n",line_cnt);
}

void Inou_cfg::generate(vector<const LGraph *> out) {
  if (out.size() == 1) {
    lgraph_2_cfg(out[0], opack.cfg_output);
  }
  else {
    for (const auto &g : out) {
      string file = g->get_name() + "_" + opack.cfg_output;
      lgraph_2_cfg(g, file);
    }
  }
}

//tmp zone
//str4num_1 = (*(words.begin())).substr(1);//get substring s[1:end]
//int num1 = std::stoi(str4num_1);
//fmt::print("num1 number is {}\n", num1);

//str4num_2 = (*(words.begin()+1)).substr(1);//get substring s[1:end]
//int num2 = std::stoi(str4num_2);
//fmt::print("num2 number is {}\n", num2);

//for(vector<string>::size_type i = 0; i != words.size(); i++){
//  if(i == words.size()-1)
//    dfg_data = dfg_data + words[i];//no final space stored
//  else
//    dfg_data = dfg_data + words[i] + " ";
//}
/*
 * no need to care about target of if-end statement inside a loop
 * deal with it later in DFG
//II. connect body end node to for end node
for(auto i = 0; i< id_nbr_null.size(); i++){
  fmt::print("con{}\n",id_nbr_null[i]);
  src_nid = *(id_nbr_null.begin()+i);
  dst_nid = name2id[w1st];//modify from w2nd to w1st
  g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, i, true));
  fmt::print("for statement, connect src_node {} to dst_node {} ----- 2\n", src_nid, dst_nid);
}
*/

/*
else if(w2nd != "null"){ //normal case:Kx->Ky, update stacks
src_nid = name2id[w1st];
dst_nid = name2id[w2nd];
g->add_edge (Node_Pin(src_nid, 0, false), Node_Pin(dst_nid, 0, true));
fmt::print("normal case connection, connect src_node {} to dst_node {} ----- 5\n", src_nid, dst_nid);

bool belong_tops = false;
Index_ID target_stack_id = src_nid;

//check equivalence bt src_nid and every top of stacks
for (auto const& x : stacks){
if(src_nid == x.second.top()){
belong_tops = true;
target_stack_id = x.first;
break;
}
}

if(!belong_tops){
stacks[target_stack_id].push(src_nid);
stacks[target_stack_id].push(dst_nid);
}
else
stacks[target_stack_id].push(dst_nid);

}
*/







