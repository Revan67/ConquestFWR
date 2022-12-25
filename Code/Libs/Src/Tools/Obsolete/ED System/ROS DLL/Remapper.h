// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Remapper_h
#define Remapper_h
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// TARemapClass
// --------------------------------------------------------------------------
template<class TRemapBaseType>
class TARemapClass
{
	public:
		virtual void Remap(const TRemapBaseType& value) const = 0;

	protected:
		TARemapClass()
		{
		};
};
// --------------------------------------------------------------------------
// TRemapClass
// --------------------------------------------------------------------------
template<class TRemapBaseType, class TRemapDescendantType>
class TRemapClass: public TARemapClass<TRemapBaseType>
{
	public:
		TRemapClass(TRemapDescendantType* instanceToRemap)
		: mInstanceToRemap(instanceToRemap)
		{
		}

	protected:
		virtual void Remap(const TRemapBaseType& value) const
		{
			ASSERT(dynamic_cast<TRemapDescendantType*>(&value));

			*mInstanceToRemap = dynamic_cast<TRemapDescendantType&>(value);
		}

	private:
		TRemapDescendantType*	mInstanceToRemap;
};
// --------------------------------------------------------------------------
// Remapper
// --------------------------------------------------------------------------
template <class TID, class TRemapBaseType>
class Remapper
{
	private:
		class RemapList;

	public:
		typedef std::list<RemapList> RemapListCollection;

		static void Add(const TID& id, TARemapClass<TRemapBaseType>* remapper)
		{
			RemapListCollection::iterator		begin = mRemapCollection.begin();
			const RemapListCollection::iterator	end = mRemapCollection.end();

			while(begin != end)
			{
				if(begin->GetID() == id)
				{
					begin->Add(remapper);
					return;
				}

				++begin;
			}

			// The id is not in the collection. Add a new list to the collection
			mRemapCollection.push_back(RemapList(id));
			mRemapCollection.back().Add(remapper);
		}

		static void RemapForID(const TID& id, const TRemapBaseType& value)
		{
			RemapListCollection::iterator		begin = mRemapCollection.begin();
			const RemapListCollection::iterator	end = mRemapCollection.end();

			while(begin != end)
			{
				if(begin->GetID() == id)
				{
					begin->RemapAll(value);
					return;
				}

				++begin;
			}

			ASSERT(0);	// ID not found!
		}

		static void RemapAtIndex(unsigned int idIndex, const TRemapBaseType& value)
		{
			ASSERT(idIndex < mRemapCollection.size());

			RemapListCollection::iterator	begin = mRemapCollection.begin();

			for(unsigned int idx = 0; idx < idIndex; ++idx)
			{
				++begin;
			}

			begin->RemapAll(value);
		}

		static unsigned int GetIDCount()
		{
			return mRemapCollection.size();
		}

		const TID& GetID(unsigned int idIndex)
		{
			ASSERT(idIndex < mRemapCollection.size());

			RemapListCollection::iterator	begin = mRemapCollection.begin();

			for(unsigned int idx = 0; idx < idIndex; ++idx)
			{
				++begin;
			}
				
			return begin->GetID();
		}

		static void Clear()
		{
			mRemapCollection.clear();
		}

	private:
		class RemapList
		{
			public:
				RemapList(const TID& id)
				: mID(id), mRemapped(true)
				{
				}

				~RemapList()
				{
					ASSERT(mRemapped == true);

					while(!mInstanceList.empty())
					{
						TARemapClass<TRemapBaseType>*	instance = mInstanceList.back();

						mInstanceList.pop_back();

						delete instance;
					}
				}

				void Add(TARemapClass<TRemapBaseType>* instanceToRemap)
				{
					mInstanceList.push_back(instanceToRemap);

					mRemapped = false;
				}

				const TID& GetID() const
				{
					return mID;
				}

				void RemapAll(const TRemapBaseType& value) const
				{
					ASSERT(mRemapped == false);

					InstanceList::const_iterator		begin = mInstanceList.begin();
					const InstanceList::const_iterator	end = mInstanceList.end();

					while(begin != end)
					{
						(*begin)->Remap(value);

						++begin;
					}

					mRemapped = true;
				}

			private:
				typedef std::list< TARemapClass<TRemapBaseType>* > InstanceList;

				TID				mID;
				InstanceList	mInstanceList;
				mutable bool	mRemapped;
		};


		static RemapListCollection	mRemapCollection;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif