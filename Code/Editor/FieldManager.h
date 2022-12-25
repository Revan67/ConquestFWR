#pragma once

struct IObject;
struct IFileSystem;

struct IFieldManager : public IDAComponent
{
	virtual bool Reset( System* _system ) = 0;

	virtual bool Insert( IObject* _field ) = 0;

	virtual bool Save( IFileSystem& _fileSystem ) = 0;

	virtual bool Load( IFileSystem& _fileSystem ) = 0;
};