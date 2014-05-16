#ifndef __FILTER_H_ef75ff09aa82fe17c4c49924c355abeab72d7b87__
#define __FILTER_H_ef75ff09aa82fe17c4c49924c355abeab72d7b87__

#include "types.h"
#include "structs/candidate.h"

namespace diagnosis {
namespace structs {
class t_filter {
public:
    t_filter ();

    t_id next (t_id id) const;

    t_id next_filtered (t_id id) const;

    t_id get_last () const;

    t_id get_filtered_count () const;

    bool is_filtered (t_id component) const;
    bool is_transaction (t_transaction_id transaction) const;

    void filter (const t_filter & filter);
    void filter (t_id component);
    void filter_all (const t_candidate & candidate);
    void filter_all_but (const t_candidate & candidate);


    void unfilter (const t_filter & filter);
    void unfilter (t_id component);

    void resize (t_id size);
private:
    std::vector<t_transaction_id> _filter;

    t_count filtered_count;
};

class t_filter_iterator {
public:
    t_filter_iterator (t_count max,
                       const t_filter * filter=NULL);


    t_id next ();
    t_id get () const;
    void set (t_id id);


private:

    t_count max;
    t_id id;
    const t_filter * filter;
};
}
}

#endif