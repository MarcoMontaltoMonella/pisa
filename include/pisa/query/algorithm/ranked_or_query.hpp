#pragma once

#include <string>
#include <vector>
#include "query/queries.hpp"

namespace pisa {

template <typename Index, typename WandType>
struct ranked_or_query {
    typedef bm25 scorer_type;

    ranked_or_query(Index const &index, WandType const &wdata, uint64_t k, uint64_t max_docid)
        : m_index(index), m_wdata(&wdata), m_topk(k), m_max_docid(max_docid) {}

    template <typename Cursor>
    uint64_t operator()(std::vector<Cursor> &&cursors) {
        m_topk.clear();
        if (cursors.empty()) return 0;
        uint64_t cur_doc =
            std::min_element(cursors.begin(), cursors.end(),
                             [](Cursor const &lhs, Cursor const &rhs) {
                                 return lhs.docs_enum.docid() < rhs.docs_enum.docid();
                             })
                ->docs_enum.docid();

        while (cur_doc < m_max_docid) {
            float    score    = 0;
            float    norm_len = m_wdata->norm_len(cur_doc);
            uint64_t next_doc = m_max_docid;
            for (size_t i = 0; i < cursors.size(); ++i) {
                if (cursors[i].docs_enum.docid() == cur_doc) {
                    score += cursors[i].q_weight *
                             scorer_type::doc_term_weight(cursors[i].docs_enum.freq(), norm_len);
                    cursors[i].docs_enum.next();
                }
                if (cursors[i].docs_enum.docid() < next_doc) {
                    next_doc = cursors[i].docs_enum.docid();
                }
            }

            m_topk.insert(score);
            cur_doc = next_doc;
        }

        m_topk.finalize();
        return m_topk.topk().size();
    }

    std::vector<std::pair<float, uint64_t>> const &topk() const { return m_topk.topk(); }

   private:
    Index const &   m_index;
    WandType const *m_wdata;
    topk_queue      m_topk;
    uint64_t        m_max_docid;
};

}  // namespace pisa