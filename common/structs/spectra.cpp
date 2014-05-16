#include "spectra.h"

#include "trie.h"

#include <boost/foreach.hpp>
#include <cmath>
#include <iomanip>

namespace diagnosis {
namespace structs {
t_probability t_spectra::get_activation_rate (const t_spectra_filter * filter) const {
    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);

    t_probability hit_count = 0;


    while (it.transaction.next())
        while (it.component.next())
            hit_count += get_activations(it.component.get(), it.transaction.get()) ? 1 : 0;

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

    tmp.components.filter_all(suspicious);

    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          &tmp);

    while (it.transaction.next()) {
        if (!is_error(it.transaction.get())) // TODO: Improve performance by maintaining a filter of all failing transactions

            continue;

        while (it.component.next()) {
            if (is_active(it.component.get(), it.transaction.get())) {
                tmp.components.filter(it.component.get());
                suspicious.insert(it.component.get());
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


    while (it.transaction.next()) {
        bool hit = false;

        if (!is_error(it.transaction.get())) // TODO: Improve performance by maintaining a filter of all failing transactions

            continue;

        BOOST_FOREACH(t_component_id c, candidate) {
            if (get_activations(c, it.transaction.get())) {
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


    while (it.transaction.next()) {
        bool hit = false;

        if (!is_error(it.transaction.get())) // TODO: Improve performance by maintaining a filter of all failing transactions

            continue;

        it.component.set(0);

        while (!hit && it.component.next())
            hit = is_active(it.component.get(), it.transaction.get());

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
    while (it.transaction.next())
        if (is_error(it.transaction.get()))
            return false;

    return true;
}

bool t_spectra::get_invalid (t_invalid_transactions & ret,
                             const t_spectra_filter * filter) const {
    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);


    ret.clear();

    while (it.transaction.next()) {
        bool hit = false;

        if (!is_error(it.transaction.get())) // TODO: Improve performance by maintaining a filter of all failing transactions

            continue;

        it.component.set(0);

        while (!hit && it.component.next())
            hit = is_active(it.component.get(), it.transaction.get());

        if (!hit)
            ret.insert(it.transaction.get());
    }

    assert(((bool) ret.size()) == is_invalid(filter));
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
    while (it.transaction.next()) {
        t_conflict_size s;
        s.first = 0;
        s.second = it.transaction.get();

        while (it.component.next()) {
            if (is_active(it.component.get(),
                          it.transaction.get()))
                s.first++;
        }

        conflict_sizes.push_back(s);

        if (!is_error(it.transaction.get()))
            f.transactions.filter(it.transaction.get());
    }

    sort(conflict_sizes.begin(), conflict_sizes.end());

    // Filter redundant conflicts
    BOOST_FOREACH(auto & s,
                  conflict_sizes) {
        t_candidate conflict;


        while (it.component.next())
            if (is_active(it.component.get(), s.second))
                conflict.insert(it.component.get());

        if (!t.add(conflict))
            f.transactions.filter(s.second);
    }

    // Filter irrelevant components
    // TODO: Move to a separate function
    while (it.component.next()) {
        bool hit = false;
        it.transaction.set(0);

        while (!hit && it.transaction.next())
            hit = is_active(it.component.get(), it.transaction.get());

        if (!hit)
            f.components.filter(it.component.get());
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
    assert(filter ? (filter->components.size() <= get_component_count()) : true);
    assert(filter ? (filter->transactions.size() <= get_transaction_count()) : true);

    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);

    t_count width_comp = log10(get_component_count());
    t_count width_tran = 2 + log10(get_transaction_count());

    t_count max_count = 0;

    while (it.transaction.next())
        while (it.component.next())
            max_count = std::max(max_count, get_activations(it.component.get(), it.transaction.get()));

    width_comp = std::max(width_comp, (t_count) log10(max_count)) + 2;

    out << std::setw(width_tran + 1) << "|";

    while (it.component.next())
        out << std::setw(width_comp) << it.component.get() << " |";

    out << "|Err (f)\n";

    char fill = out.fill('-');
    out << std::setw(width_tran + 1) << "+";

    while (it.component.next())
        out << std::setw(width_comp + 2) << "+";

    out << "+-------\n";
    out.fill(fill);

    while (it.transaction.next()) {
        out << std::setw(width_tran) << std::left << it.transaction.get() << std::setw(0) << "|" << std::right;

        while (it.component.next())
            out << std::setw(width_comp) << get_activations(it.component.get(), it.transaction.get()) << " |";

        out << "| " << is_error(it.transaction.get()) << "  (" << get_error(it.transaction.get()) << ")\n";
    }

    fill = out.fill('-');
    out << std::setw(width_tran + 1) << "+";

    while (it.component.next())
        out << std::setw(width_comp + 2) << "+";

    out << "+-------\n";
    out.fill(fill);


    return out;
}

std::ostream & t_spectra::write (std::ostream & out,
                                 const t_spectra_filter * filter) const {
    assert(filter ? (filter->components.size() <= get_component_count()) : true);
    assert(filter ? (filter->transactions.size() <= get_transaction_count()) : true);

    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);
    out << get_component_count(filter) << " " << get_transaction_count(filter) << "\n";

    while (it.transaction.next()) {
        while (it.component.next())
            out << get_activations(it.component.get(), it.transaction.get()) << " ";

        out << " " << get_error(it.transaction.get()) << "\n";
    }

    return out;
}

std::istream & t_spectra::read (std::istream & in) {
    throw;
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

    t_spectra_iterator it(get_component_count(),
                          get_transaction_count(),
                          filter);


    while (it.transaction.next())
        if (is_error(it.transaction.get()))
            total_errors++;

    return total_errors;
}

t_count t_basic_spectra::get_component_count (const t_spectra_filter * filter) const {
    assert(!filter || filter->components.size() <= component_count);

    if (filter)
        return component_count - filter->components.get_filtered_count();

    return component_count;
}

t_count t_basic_spectra::get_transaction_count (const t_spectra_filter * filter) const {
    assert(!filter || filter->transactions.size() <= transaction_count);

    if (filter)
        return transaction_count - filter->transactions.get_filtered_count();

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

namespace std {
std::istream & operator >> (std::istream & in, diagnosis::structs::t_spectra & spectra) {
    return spectra.read(in);
}

std::ostream & operator << (std::ostream & out, const diagnosis::structs::t_spectra & spectra) {
    return spectra.write(out);
}
}