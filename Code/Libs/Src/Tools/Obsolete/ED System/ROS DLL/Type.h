// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef TypeMatch_h
#define TypeMatch_h
// --------------------------------------------------------------------------
template<class TBaseType>
class AType
{
    public:
        virtual bool Matches(const TBaseType& kObjR) const = 0;
};
// --------------------------------------------------------------------------
template<class TBaseType, class TDescendantType>
class Type: public AType<TBaseType>
{
    public:
        virtual bool Matches(const TBaseType& kObjR) const
        {
            return dynamic_cast<const TDescendantType*>(&kObjR) != NULL;
        }
};
// --------------------------------------------------------------------------
template<class TBaseType>
class ATypeFactory
{
    public:
        virtual bool Matches(const TBaseType& kObjR) const = 0;
        virtual TBaseType Manufacture() const = 0;
};
// --------------------------------------------------------------------------
template<class TBaseType, class TDescendantType>
class TypeFactory: public ATypeFactory<TBaseType>
{
    public:
        virtual bool Matches(const TBaseType& kObjR) const
        {
            return dynamic_cast<const TDescendantType*>(&kObjR) != NULL;
        }
        virtual TBaseType Manufacture() const
        {
        	return new TDescendantType;
        }
};
// --------------------------------------------------------------------------
#endif
