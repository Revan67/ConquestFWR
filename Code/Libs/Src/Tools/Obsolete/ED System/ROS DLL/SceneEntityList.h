// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneEntityList_h
#define SceneEntityList_h
// --------------------------------------------------------------------------
#include "StringType.h"
#include "Links.h"
#include "ASceneEntity.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
// --------------------------------------------------------------------------
//	SceneEntityList
// --------------------------------------------------------------------------
/**# :[Description = "Deletes all scene entities it lists when it destructs."] [Note = "Design so that the class can be reused for stocking objects for game levels."] */
class CPP_DECL SceneEntityList
{
	public:
	    typedef AggList<ASceneEntity*>	SceneEntityPList;
        typedef SceneEntityPList::const_iterator ConstIterator;
        typedef SceneEntityPList::iterator Iterator;

        class ExDuplicateSceneEntityName: public std::exception
        {
        };

        class ExInvalidSceneEntityInStream: public std::exception
        {
			public:
				ExInvalidSceneEntityInStream(const ROSString& entityTypeName)
				: mMessage(ROSString("Unrecognized Scene Entity Type: ") + entityTypeName)
				{
				}
					
				virtual const char* what() const throw()
				{
				  return mMessage.c_str();
				}

			private:
				ROSString	mMessage;
        };

        class ExSceneEntityNotFound: public std::exception
        {
        };

        class ExSceneEntityNameAlreadyInUse: public std::exception
        {
        };

        SceneEntityList(Scene& scene);
        ~SceneEntityList();
        /**# :[Description = "Throws exception if another entity with the same name is already in the list."] */
        void AddSceneEntity(ASceneEntity& sceneEntityR);
        void RemoveSceneEntity(const ASceneEntity& sceneEntityR);
        ASceneEntity* GetSceneEntity(const ROSString& kEntityNameR) const;

        /**# :[Description = "Throws exception ExSceneEntityNotFound if the entity to rename cannot be found. Throws exception ExSceneEntityNotFound if an entity with the specified name does not exist. Throws exception ExSceneEntityNameAlreadyInUse if another entity with the new name is already in the list."] */
        void RenameSceneEntity(const ROSString& kEntityToRenameR, const ROSString& kNewNameR);

        void Write(std::ostream& ostreamR) const;
        void Read(std::istream& istreamR);

        ConstIterator	Begin() const;
        Iterator	    Begin();
        ConstIterator	End() const;
        Iterator	    End();

        unsigned int	Size() const;

	private :
		void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);

        /**# :[Cardinalities = "0..n/"] */
        SceneEntityPList	mSceneEntityPL;
		Scene&				mScene;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::SceneEntityList& list)
{
	list.Write(oStream);

	return oStream;
}
//---------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::SceneEntityList& list)
{
 	list.Read(iStream);

	return iStream;
}
//---------------------------------------------------------------------------
#endif

