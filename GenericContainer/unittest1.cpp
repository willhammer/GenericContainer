#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GenericStorage
{
    enum class StoreType : int
    {
        StoreType_Generic = 0,
        StoreType_Specific = 1
    };

    class GenericObjectStore
    {
    protected:
        void* ptr;
        
    public:
        virtual StoreType GetStoreType()
        {
            return StoreType::StoreType_Generic;
        }

        void SetPtr(void* dataPointer)
        {
            ptr = dataPointer;
        }

        void* const GetPtr() const
        {
            return ptr;
        }

        GenericObjectStore(const GenericObjectStore& other) = delete;
        void operator=(const GenericObjectStore& other) = delete;
        
        GenericObjectStore()
        {
            ptr = nullptr;
        }

        virtual const std::type_info& GetTypeInfo()
        {
            return typeid(void);
        }
    };
    
    template<typename T>
    class TypeSpecificObjectStore : public GenericObjectStore
    {

    public:

        StoreType GetStoreType() override
        {
            return StoreType::StoreType_Specific;
        }

        const std::type_info& GetTypeInfo() override
        {
            return typeid(T);
        }

        T* GetValue()
        {
            return (T*)ptr;
        }
    };
    
    class GenericContainer
    {
    private:
        std::unordered_map<std::string, std::shared_ptr<GenericObjectStore>> vars;

    public:
        template<typename T>
        void Add(std::string name, T* var)
        {
            void* varAsVoidPtr = (void*)(var);

            TypeSpecificObjectStore<T>* newStore = new TypeSpecificObjectStore<T>();
            assert(newStore != nullptr);

            newStore->SetPtr(varAsVoidPtr);
            vars.emplace(name, std::shared_ptr<TypeSpecificObjectStore<T>>(newStore));
        }

        std::shared_ptr<GenericObjectStore> GetGenericStoreAt(std::string name)
        {
            return vars.at(name);
        }

        size_t Size()
        {
            return vars.size();
        }
    };

    template<typename T>
    T* const GetValuePtr(const std::shared_ptr<GenericObjectStore>& storeContainer)
    {
        GenericObjectStore* genericStorePtr = storeContainer.get();
        if(genericStorePtr == nullptr)
            throw std::runtime_error("Data store object pointer is null. This should never happen!");

        if (genericStorePtr->GetStoreType() == StoreType::StoreType_Generic)
            throw std::runtime_error("GenericObjectStore should not be used in itself. Use TypeSpecificObjectStore.");

        auto specificStore = reinterpret_cast<TypeSpecificObjectStore<T>*>(genericStorePtr);
        return specificStore->GetValue();
    }
}

namespace GenericContainerTest
{
    using namespace GenericStorage;

	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(GenericContainerExists)
		{
            GenericContainer container;
            Assert::AreEqual(container.Size(), (size_t)0);
		}

        TEST_METHOD(GenericContainerContainsSeveralTypes)
        {
            class RandomClass
            {
                const int a = 5;
                float b;
            };

            GenericContainer container;

            int one = 1;
            double two = 2.0;
            float three = 3.0f;

            container.Add("one", &one);
            container.Add("two", &two);
            container.Add("three", &three);
            RandomClass rc;
            container.Add("RandomClass", &rc);

            Assert::AreEqual(container.Size(), (size_t)4);
        }
        
        TEST_METHOD(GenericContainer_Index0IsInt)
        {
            GenericContainer container;

            int one = 1;
            double two = 2.0;
            float three = 3.0f;

            container.Add("one", &one);
            container.Add("two", &two);
            container.Add("three", &three);

            auto genericStoreAtIndex0 = container.GetGenericStoreAt(std::string("one"));
            auto& valueInStore = *(reinterpret_cast<int*>(genericStoreAtIndex0->GetPtr()));
            
            Assert::IsTrue(typeid(valueInStore) == typeid(int));
        }

        TEST_METHOD(GenericContainer_Index1IsDouble)
        {
            int one = 1;
            double two = 2.0;
            float three = 3.0f;

            GenericContainer container;
            container.Add("one", &one);
            container.Add("two", &two);
            container.Add("three", &three);

            auto genericStoreAtIndex1 = container.GetGenericStoreAt(std::string("two"));
            auto& valueInStore = *(reinterpret_cast<double*>(genericStoreAtIndex1->GetPtr()));
            Assert::IsTrue(typeid(valueInStore) == typeid(double));
            Assert::IsTrue(valueInStore == 2.0);
        }

        TEST_METHOD(GenericContainer_GetObjPtr)
        {
            int one = 1;
            double two = 2.0;
            float three = 3.0f;

            GenericContainer container;
            container.Add("one", &one);
            container.Add("two", &two);
            container.Add("three", &three);

            GenericObjectStore* genericStoreAtIndex0 = container.GetGenericStoreAt(std::string("one")).get();
            TypeSpecificObjectStore<int>* specificStoreAtIndex0 = reinterpret_cast<TypeSpecificObjectStore<int>*>(genericStoreAtIndex0);
            auto value = specificStoreAtIndex0->GetValue();
            
            Assert::IsTrue(typeid(value) == typeid(int*));
            Assert::IsTrue(*value == 1);
        }

        TEST_METHOD(GenericContainer_GetObjPtr_GenericAccess)
        {
            int one = 1;
            double two = 2.0;
            float three = 3.0f;

            GenericContainer container;
            container.Add("one", &one);
            container.Add("two", &two);
            container.Add("three", &three);

            std::shared_ptr<GenericObjectStore> genericStoreAtIndex0 = container.GetGenericStoreAt(std::string("one"));
            auto valuePtr = GetValuePtr<int>(genericStoreAtIndex0);

            Assert::IsTrue(typeid(valuePtr) == typeid(int*));
            Assert::IsTrue(*valuePtr == 1);
        }

        TEST_METHOD(GenericContainer_ModifyOne)
        {
            int one = 1;
            double two = 2.0;
            float three = 3.0f;

            GenericContainer container;
            container.Add("one", &one);
            container.Add("two", &two);
            container.Add("three", &three);

            auto genericStoreAtIndex0 = container.GetGenericStoreAt(std::string("one"));
            auto& valueInStore = *(reinterpret_cast<int*>(genericStoreAtIndex0->GetPtr()));
            valueInStore += 1;

            Assert::IsTrue(one == 2);
        }
    };
}