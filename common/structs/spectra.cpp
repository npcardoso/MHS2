#include "spectra.h"

#include "spectra_iterator.h"
#include "trie.h"

#include <boost/foreach.hpp>

namespace diagnosis {
namespace structs {
using namespace diagnosis::structs;
// FIXME: !!!!Experimental!!!!
t_probability t_spectra::get_entropy (const t_spectra_filter * filter) const {
    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);

    t_count total_components = get_component_count() - (filter ? filter->get_filtered_component_count() : 0);
    std::vector<t_count> col_ones(total_components, 0);

    t_count total_transactions = get_transaction_count() - (filter ? filter->get_filtered_transaction_count() : 0);
    std::vector<t_count> row_ones(total_transactions, 0);


    // Count activations
    for (int i = 0; it.next_component(); i++)
        for (int j = 0; it.next_transaction(); j++)
            if (get_activations(it.get_component(), it.get_transaction())) {
                col_ones[i]++;
                row_ones[j]++;
            }


    // Calculate entropy
    t_probability entropy = 0;

    for (int i = 0; it.next_component(); i++)
        for (int j = 0; it.next_transaction(); j++) {
            t_probability tmp;

            if (get_activations(it.get_component(), it.get_transaction()))
                tmp = col_ones[i] * row_ones[j];
            else
                tmp = (total_transactions - col_ones[i]) * (total_components - row_ones[j]);

            tmp /= (t_probability) (total_components * total_transactions);
            entropy -= tmp * log(tmp);
        }


    return entropy;
}

t_probability t_spectra::get_activation_rate (const t_spectra_filter * filter) const {
    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);

    t_probability hit_count = 0;


    while (it.next_transaction())
        while (it.next_component())
            hit_count += get_activations(it.get_component(), it.get_transaction()) ? 1 : 0;

    return hit_count / (get_component_count(filter) * get_transaction_count(filter));
}

t_probability t_spectra::get_error_rate (const t_spectra_filter * filter) const {
    return get_error_count(filter) / (t_probability) get_transaction_count(filter);
}

t_count t_spectra::get_suspicious_components_count (const t_spectra_filter * filter) const {
    t_candidate tmp;


    return get_suspicious_components_count(tmp, filter);
}

t_count t_spectra::get_suspicious_components_count (t_candidate & suspicious,
                                                    const t_spectra_filter * filter) const {
    t_spectra_filter tmp;


    if (filter)
        tmp = *filter;

    tmp.filter_all_components(suspicious);

    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          &tmp);

    while (it.next_transaction()) {
        if (!is_error(it.get_transaction())) // TODO: Improve performance by maintaining a filter of all failing transactions

            continue;

        while (it.next_component()) {
            if (is_active(it.get_component(), it.get_transaction())) {
                tmp.filter_component(it.get_component());
                suspicious.insert(it.get_component());
            }
        }
    }

    return suspicious.size();
}

bool t_spectra::is_active (t_component_id component,
                           t_transaction_id transaction) const {
    return get_activations(component, transaction) > 0;
}

bool t_spectra::is_candidate (const t_candidate & candidate,
                              const t_spectra_filter * filter) const {
    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);


    while (it.next_transaction()) {
        bool hit = false;

        if (!is_error(it.get_transaction())) // TODO: Improve performance by maintaining a filter of all failing transactions

            continue;

        BOOST_FOREACH(t_component_id c, candidate) {
            if (get_activations(c, it.get_transaction())) {
                hit = true;
                break;
            }
        }

        if (!hit)
            return false;
    }

    return true;
}

// FIXME: Not very efficient
bool t_spectra::is_minimal_candidate (const t_candidate & candidate,
                                      const t_spectra_filter * filter) const {
    if (!is_candidate(candidate, filter))
        return false;

    t_candidate tmp = candidate;
    BOOST_FOREACH(t_component_id c, candidate) {
        tmp.erase(c);

        if (is_candidate(tmp, filter))
            return false;

        tmp.insert(c);
    }
    return true;
}

bool t_spectra::is_invalid (const t_spectra_filter * filter) const {
    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);


    while (it.next_transaction()) {
        bool hit = false;

        if (!is_error(it.get_transaction())) // TODO: Improve performance by maintaining a filter of all failing transactions

            continue;

        it.set_component(0);

        while (!hit && it.next_component())
            hit = is_active(it.get_component(), it.get_transaction());

        if (!hit)
            return true;
    }

    return false;
}

bool t_spectra::is_all_pass (const t_spectra_filter * filter) const {
    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);


    // TODO: Improve performance by maintaining a filter of all failing transactions
    while (it.next_transaction())
        if (is_error(it.get_transaction()))
            return false;

    return true;
}

bool t_spectra::get_invalid (t_invalid_transactions & ret,
                             const t_spectra_filter * filter) const {
    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);


    ret.clear();

    while (it.next_transaction()) {
        bool hit = false;

        if (!is_error(it.get_transaction())) // TODO: Improve performance by maintaining a filter of all failing transactions

            continue;

        it.set_component(0);

        while (!hit && it.next_component())
            hit = is_active(it.get_component(), it.get_transaction());

        if (!hit)
            ret.insert(it.get_transaction());
    }

    assert(((bool)ret.size()) == is_invalid(filter));
    return ret.size();
}

t_ptr<t_spectra_filter> t_spectra::get_minimal_conflicts (const t_spectra_filter * filter) const {
    t_ptr<t_spectra_filter> f;
    if (filter)
        f = t_ptr<t_spectra_filter> (new t_spectra_filter(*filter));
    else
        f = t_ptr<t_spectra_filter> (new t_spectra_filter());

    get_minimal_conflicts(*f);
    return f;
}

void t_spectra::get_minimal_conflicts (t_spectra_filter & f) const {
    t_trie t;

    typedef std::pair<t_count, t_transaction_id> t_conflict_size;
    std::vector<t_conflict_size> conflict_sizes;

    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          &f);

    // Filter non-error transactions
    // TODO: Move to a separate function
    while (it.next_transaction()) {
        t_conflict_size s;
        s.first = 0;
        s.second = it.get_transaction();

        while (it.next_component()) {
            if (is_active(it.get_component(),
                          it.get_transaction()))
                s.first++;
        }

        conflict_sizes.push_back(s);

        if (!is_error(it.get_transaction()))
            f.filter_transaction(it.get_transaction());
    }

    sort(conflict_sizes.begin(), conflict_sizes.end());

    // Filter redundant conflicts
    BOOST_FOREACH(auto & s,
                  conflict_sizes) {
        t_candidate conflict;

        while (it.next_component())
            if (is_active(it.get_component(), s.second))
                conflict.insert(it.get_component());

        if (!t.add(conflict))
            f.filter_transaction(s.second);
    }

    // Filter irrelevant components
    // TODO: Move to a separate function
    while (it.next_component()) {
        bool hit = false;
        it.set_transaction(0);

        while (!hit && it.next_transaction())
            hit = is_active(it.get_component(), it.get_transaction());

        if (!hit)
            f.filter_component(it.get_component());
    }
}

void t_spectra::set_component_count (t_count component_count) {
    set_count(component_count, get_transaction_count());
}

void t_spectra::set_transaction_count (t_count transaction_count) {
    set_count(get_component_count(), transaction_count);
}

std::ostream & t_spectra::print (std::ostream & out,
                                 const t_spectra_filter * filter) const {
    throw;
}

std::ostream & t_spectra::write (std::ostream & out,
                                 const t_spectra_filter * filter) const {
    throw;
}

std::istream & t_spectra::read (std::istream & in) {
    throw;
}

std::istream & operator >> (std::istream & in, t_spectra & spectra) {
    return spectra.read(in);
}

std::ostream & operator << (std::ostream & out, const t_spectra & spectra) {
    return spectra.write(out);
}

t_basic_spectra::t_basic_spectra () {
    set_transaction_count(0);
    set_component_count(0);
}

t_basic_spectra::t_basic_spectra (t_count component_count,
                                  t_count transaction_count) {
    set_transaction_count(transaction_count);
    this->transaction_count = transaction_count;
    set_component_count(component_count);
}

t_count t_basic_spectra::get_error_count (const t_spectra_filter * filter) const {
    // FIXME: Improve performance
    t_count total_errors = 0;


    if (filter) {
        t_id i = filter->next_transaction(0);

        while (i < get_transaction_count()) {
            if (is_error(i))
                total_errors++;

            i = filter->next_transaction(i);
        }
    }
    else {
        for (t_id i = 1; i <= get_transaction_count(); i++)
            if (is_error(i)) // TODO: Improve performance by maintaining a filter of all failing transactions

                total_errors++;
    }

    return total_errors;
}

t_count t_basic_spectra::get_component_count (const t_spectra_filter * filter) const {
    assert(!filter || filter->get_last_component() <= component_count);

    if (filter)
        return component_count - filter->get_filtered_component_count();

    return component_count;
}

t_count t_basic_spectra::get_transaction_count (const t_spectra_filter * filter) const {
    assert(!filter || filter->get_last_transaction() <= transaction_count);

    if (filter)
        return transaction_count - filter->get_filtered_transaction_count();

    return transaction_count;
}

void t_basic_spectra::set_count (t_count component_count,
                                 t_count transaction_count) {
    this->component_count = component_count;

    this->transaction_count = transaction_count;

    errors.resize(transaction_count, 0);
}

t_error t_basic_spectra::get_error (t_transaction_id transaction) const {
    assert(transaction > 0);
    assert(transaction <= transaction_count);

    return errors[transaction - 1];
}

bool t_basic_spectra::is_error (t_transaction_id transaction) const {
    assert(transaction > 0);
    assert(transaction <= transaction_count);

    return get_error(transaction) >= 1; // TODO: arbitrary threshold
}

void t_basic_spectra::set_error (t_transaction_id transaction,
                                 t_error error) {
    assert(transaction > 0);
    assert(transaction <= transaction_count);
    assert(error >= 0);
    assert(error <= 1);

    errors[transaction - 1] = error;
}
}
}
