#include <iostream>

#include "query/queries.hpp"
#include "CLI/CLI.hpp"
#include "spdlog/spdlog.h"

#include "mio/mmap.hpp"

#include "succinct/mapper.hpp"

using namespace pisa;

template <typename Index>
void live_eval(const Index& index, term_id_vec terms) {
    typedef typename Index::document_enumerator enum_type;
    std::vector<enum_type>                      enums;
    enums.reserve(terms.size());

    for (auto term : terms) {
        enums.push_back(index[term]);
    }
    
    uint64_t candidate = 0;    
    //while (candidate < index.num_docs()) {
    //    enums[0].next_geq(candidate);
    //    std::cout << enums[0].docid() << " ";
    //    candidate = enums[0].docid();
    //}

    for (const auto& en : enums) {
	    std::cout << en.size() << " ";
    }
    std::cout << std::endl;
}

template <typename IndexType, typename WandType>
void livendead(const std::string& index_filename,
		const std::optional<std::string>& wand_data_filename,
		const std::vector<term_id_vec> &queries) {
    // loading the index and opening with wand data 
    IndexType index;
    spdlog::info("Loading index from {}", index_filename);
    mio::mmap_source m(index_filename.c_str());
    mapper::map(index, m);
   
    WandType wdata;
    mio::mmap_source md;
    if (wand_data_filename) {
        std::error_code error;
        md.map(wand_data_filename.value(), error);
        if(error){
            std::cerr << "error mapping file: " << error.message() << ", exiting..." << std::endl;
            throw std::runtime_error("Error opening file");
        }
        mapper::map(wdata, md, mapper::map_flags::warmup);
    }
    
    for (const auto& query : queries) {
        live_eval(index, query); 

    }
}



using wand_raw_index = wand_data<bm25, wand_data_raw<bm25>>;
using opt_index =
    freq_index<partitioned_sequence<>, positive_sequence<partitioned_sequence<strict_sequence>>>;
int main(int argc, const char **argv) {
    //std::string type;
    std::string index_filename;
    std::optional<std::string> wand_data_filename;
    std::optional<std::string> query_filename;
   

    CLI::App app{"queries - a tool for performing queries on an index."};
    app.add_option("-i,--index", index_filename, "Collection basename")->required();
    //app.add_option("-t,--type", type, "Index type")->required();
    app.add_option("-w,--wand", wand_data_filename, "Wand data filename");
    app.add_option("-q,--query", query_filename, "Queries filename");
    CLI11_PARSE(app, argc, argv);
 

        
    std::vector<term_id_vec> queries;
    term_id_vec q;
    if (query_filename) {
        std::filebuf fb;
        if (fb.open(query_filename.value(), std::ios::in)) {
            std::istream is(&fb);
            while (read_query(q, is))
                queries.push_back(q);
        }
    } else {
        while (read_query(q))
            queries.push_back(q);
    }

    livendead<opt_index, wand_raw_index>(index_filename, wand_data_filename, queries);
    //for(const auto& query :queries) {
    //    for(const auto& q : query) {
    //        std::cout << q << " ";
    //    }
    //    std::cout << std::endl;
    //}
}
