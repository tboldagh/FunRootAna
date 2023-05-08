// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef Access_h
#define Access_h
#include <string>
#include <limits>
#include <type_traits>
#include "TTree.h"
#include "TLeaf.h"
#include "LazyFunctionalVector.h"


#include "Math/Vector4D.h"
#include "TLorentzVector.h"
#include "assure.h"


typedef TLorentzVector TLorentz;

namespace{
	template<typename T>
	struct fillbr {
		T* data = nullptr;
		TBranch* br = nullptr;
		fillbr(TTree* t, const std::string& brname, size_t event, bool conditional = false) {

			br = t->GetBranch(brname.c_str());
			if (br == nullptr and conditional)
				return;

			assure(br != nullptr, "branch found " + brname, true);
			br->SetAddress(&data);
			assure(data != nullptr, "after set address of " + brname, true);
			br->GetEntry(event);
		}
		~fillbr() {
			if (br) { br->SetAddress(0); delete data; }
		}
		size_t size() const { return data->size(); }
		typename T::value_type at(size_t i) const { return data->at(i); }
		operator bool() const { return data != nullptr; }
	};

	template<class T>
	struct fillbrpod {
		T* data = nullptr;
		TLeaf* br = nullptr;
		fillbrpod(TTree* t, const std::string& brname, size_t event, bool conditional = false) {
			br = t->GetLeaf(brname.c_str());
			if (br == nullptr and conditional)
				return;

			assure(br != nullptr, "branch found " + brname, true);
			t->GetBranch(brname.c_str())->GetEntry(event);
			data = reinterpret_cast<T*>(br->GetValuePointer());
		}
		~fillbrpod() {
		}
		operator bool() const { return data != nullptr; }
	};

	// helper for POD arrays
	template<class T, size_t SIZE>
	struct fillbrarr {
		typedef T* DATAPTR;
		DATAPTR data = nullptr;
		TBranch* br = nullptr;
		fillbrarr(TTree* t, const std::string& brname, size_t event, DATAPTR store, bool conditional = false) {
			br = t->GetBranch(brname.c_str());
			if (br == nullptr and conditional)
				return;
			assure(br != nullptr, "branch found " + brname, true);
			t->SetBranchAddress(brname.c_str(), *data, &br);
			br->GetEntry(event);
		}
		T operator[](size_t index) const { return (*data)[index]; }
		~fillbrarr() {
		}
		operator bool() const { return data != nullptr; }
	};
}


class Access {
public:
	Access(TTree* t, size_t start = 0, size_t max = std::numeric_limits<size_t>::max()) : m_current(start), m_tree(t) {
		m_max = (max == std::numeric_limits<size_t>::max() ? m_tree->GetEntriesFast() : max);
	}

	bool exists(const std::string& name) {
		return m_tree->GetBranch(name.c_str()) != nullptr;
	}

	// get any branch, two variants available for PODs (below) and vectors
	template<typename T, typename std::enable_if<not std::is_pod<T>::value, int>::type = 0>
	T get(const std::string& name) {
		fillbr<T> br(m_tree, name, m_current);
		return *(br.data);
	}

	template<typename T, typename std::enable_if<std::is_pod<T>::value, int>::type = 0 >
	T get(const std::string& name) {
		fillbrpod<T> br(m_tree, name, m_current);
		T value = *(br.data);
		return value;
	}

	template<typename T, typename std::enable_if<not std::is_pod<T>::value, int>::type = 0>
	auto branch_view(const std::string& name) {
		auto br = fillbr<T>(m_tree, name, m_current);
		return lfv::lazy_view(*(br.data));
	}

	template<typename T, typename std::enable_if<std::is_pod<T>::value, int>::type = 0 >
	auto branch_view(const std::string& name) {
		auto br = fillbrpod<T>(m_tree, name, m_current);
		return lfv::one_own(*(br.data));
	}


	// TLorentzVector gettter, from branches named: pt, eta, phi, potentially prefixed by the string passed in the argument
	// eg. if the tree has branches electron_pt, electron_eta, electron_phi
	// the tlget("electron_") will return vector of TLorentzVectors
	std::vector<TLorentz> tlget(const std::string& brprefix);

	// looping helpers
	// they allow to process the tree in a loop like this:
	// for ( Access a(tree); a; ++a)
	operator bool()  const { return m_current < m_max; }
	void operator++() { ++m_current; }
	size_t current() const { return m_current; }
	TTree* tree() const { return m_tree; }


private:
	size_t m_current;
	TTree* m_tree{ nullptr };
	size_t m_max;
};


#define FBR(_type,_brname)     fillbr<std::vector<_type>> _brname( m_tree, #_brname , m_current);
#define COND_FBR(_type,_brname)     fillbr<std::vector<_type>> _brname( m_tree, #_brname , m_current, true);

/**
 * Helper class enabling procssing of the wrapped tree alike the lazy container
 * The tree is considered to be an infinite container (so some functions, that are anyways useless, are not available)
 * Example:
 * class MyData : public Access {...};
 * TreeView<MyData> data( treePtr );
 * data.filter(...).take(...).skip(...).foreach(...);
 * For instance to simply count the elements passing the criteria:
 * data.count( [](auto d){ return d... }); // predicate
 * Typical use is though to just do: data.foreach( ...lambda... );
 **/
template<typename AccessDerivative>
class TreeView : public lfv::FunctionalInterface<TreeView<AccessDerivative>, AccessDerivative> {
public:
	using interface = lfv::FunctionalInterface<TreeView<AccessDerivative>, AccessDerivative>;
	static constexpr bool is_permanent = false;
	static constexpr bool is_finite = false;

	TreeView(TTree* t)
		: interface(*this),
		m_access(t) {}

	template<typename F>
	void foreach_imp(F f, lfv::details::foreach_instructions = {}) const {
		for (; m_access; ++m_access) {
			const bool go = f(m_access);
			if (not go) break;
		}
	}

private:
	mutable AccessDerivative m_access;

};


/**
 * Helper class allowing to view several branches (vectors) synchronously as one object
 * The CollatedBranchesIterator is not exposed to outside and only the CollatedBranchesContainer is.
 * The idea is following:
 *  - the collection of objects is typically stored in tree as several simple branches, say Point x,y,z coordinates
 *  - they want to be viewed as one object Point with attributes x,y,z
 *  - branches are loaded into memory as separate
 *  - the CollatedContainer is made and provides way to iterate over all of the collections synchronously
 *  x vector ++++++++++++++
 *  y vector ++++++++++++++
 *  z vector ++++++++++++++
 *  iterator ^ points to these consecutive elements and deref operator of it makes Point object.
 * User needs to supply class/struct that can take branches (ideally addresses/pointers) and provides build method co construct the object.
 * @file example_analysis.cpp for an example
 * @see CollatedBranchesContainer for more documentation
 * CollatedBranchContainer can be lazy_viewed
 **/
namespace {
template<typename C >
struct CollatedBranchesIterator {
    CollatedBranchesIterator(size_t i, const C& c) : current(i), container(c) {}
    bool operator != (const CollatedBranchesIterator& end) {  return current != end.current; }
    CollatedBranchesIterator& operator++ () { current++; return *this; }
    typename C::value_type operator*() const { return container.build(current); }
    size_t current = 0;
    const C& container;
};
}


/**
 * @brief class that supports a synchronous iteration over several elementary branches
 * Assume data stored in two branches of type vector<float> x, y but we want to view them
 * as a vector<Coord> for which the x and y are members.
 * This class facilitates such access w/o ever creating vector<Coord> itself.
 * Instead the individual branches are used as a source to produce the Coord objects
 * while iterating over "pseudo" container of Coord objects.
 *
 * The class is intended to be used with CRTP patterns rather than usual inheritance.
 * @tparam VT - value type that is made of of collated branches
 * @tparam C - pseudo container of VT objects
 * it would gain access to the branches (possibly using fillbr helpers)
 * it provides build method that, given index, construct object from branches under that index
 */

template<typename C, typename VT>
class CollatedBranchesContainer {
    public:
        using value_type=VT;
        using iterator_type=CollatedBranchesIterator<CollatedBranchesContainer<C, VT>>;
        CollatedBranchesContainer(size_t s=0) : m_size(s) {}
        iterator_type begin() const { return iterator_type(0ul, *this); }
        iterator_type end() const { return iterator_type(m_size, *this); }
		/**
		 * @brief construct object from branches data
		 * @param index of data elements in branch vectors
		 * @return value_type
		 * To assure performance make sure the override is "final"
		 */
        virtual value_type build( size_t index ) const = 0;
		size_t size() const { return m_size; }
	protected:
		void updateSize(size_t s) { m_size = s; }

    private:
        size_t m_size = 0;
};


#endif // Access_h
