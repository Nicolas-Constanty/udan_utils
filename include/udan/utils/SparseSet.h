#pragma once
#include <typeindex>
#include <vector>

namespace udan
{
	namespace utils
	{
		template<typename Entity>
		class SparseSet {
		protected:
			std::vector<Entity> m_sparse;
			std::vector<Entity> m_dense;
			std::queue<Entity> m_freeEntities;
			size_t m_noEntity;

		public:
			explicit SparseSet(size_t capacity = 512) : m_sparse(capacity, capacity - 1)
			{
				m_noEntity = capacity - 1;
			}

			const std::vector<Entity>& Entities() const
			{
				return m_dense;
			}
		};


		template<typename Dataset>
		auto GetDataAtIndex(Dataset& dataset, size_t index)
		{
			return dataset.GetAtPosition(index);
		}

		template<typename Dataset>
		size_t GetDataSize(const Dataset& dataset)
		{
			return dataset.GetSize();
		}

		template<typename Entity, typename Dataset>
		bool EntityExist(Entity e, const Dataset& dataset)
		{
			return dataset.Exist(e);
		}

		template<typename Entity, typename Dataset>
		bool SwapEntity(size_t index, Entity e, Dataset& dataset)
		{
			dataset.Swap(index, e);
			return true;
		}

		template <typename Entity, class Tuple, size_t N = 0>
		const std::vector<Entity>& RuntimeGet(Tuple& tup, size_t idx) {
			if (N == idx) {
				return std::get<N>(tup).Entities();
			}

			if constexpr (N + 1 < std::tuple_size_v<Tuple>) {
				return RuntimeGet<Entity, Tuple, N + 1>(tup, idx);
			}
		}

		template<typename Entity, typename ...Datasets>
		class DataSetView
		{
			size_t m_start;
			size_t m_end;
			std::tuple<Datasets& ...> m_datasets;

		public:
			DataSetView(const std::vector<Entity>& m_entities, Datasets& ...datasets) : m_datasets({ std::forward<Datasets>(datasets) ... })
			{

				std::vector<Entity> matches;
				//{
				//	const udan::utils::Timer timer;
				//	for (const auto entity : m_entities)
				//	{
				//		std::vector<bool> exists = { EntityExist(entity, std::get<Datasets&>(m_datasets)) ... };
				//		bool match = true;
				//		for (bool ee : exists)
				//		{
				//			if (!ee)
				//			{
				//				match = false;
				//				break;
				//			}
				//		}
				//		if (match)
				//		{
				//			matches.push_back(entity);
				//		}
				//	}
				//	const auto execTime = timer.GetDeltaTime();
				//	LOG_INFO("Match Time: (fps {}) {}s", 1.0 / execTime, execTime);
				//}

				{
					const udan::utils::Timer timer;
					std::vector<size_t> sizes = { GetDataSize(std::get<Datasets&>(m_datasets)) ... };
					size_t result = UINT_MAX;
					int index = 0;
					for (int i = 0; i < sizes.size(); ++i)
					{
						if (sizes[i] < result)
						{
							result = sizes[i];
							index = i;
						}
					}
					auto entities = RuntimeGet<Entity, decltype(m_datasets)>(m_datasets, index);
					for (const auto entity : entities)
					{
						std::vector<bool> exists = { EntityExist(entity, std::get<Datasets&>(m_datasets)) ... };
						bool match = true;
						for (bool ee : exists)
						{
							if (!ee)
							{
								match = false;
								break;
							}
						}
						if (match)
						{
							matches.push_back(entity);
						}
					}
					const auto execTime = timer.GetDeltaTime();
					LOG_DEBUG("Match Time: (fps {}) {}s", 1.0 / execTime, execTime);
				}

				{
					const udan::utils::Timer timer;
					m_start = 0;
					m_end = matches.size();
					for (size_t i = 0; i < matches.size(); ++i)
					{
						std::vector<bool> exists = { SwapEntity(i, matches[i], std::get<Datasets&>(m_datasets)) ... };
					}
					const auto execTime = timer.GetDeltaTime();
					LOG_DEBUG("Sort Time: (fps {}) {}s", 1.0 / execTime, execTime);
				}
			}

			auto Get(size_t index)
			{
				return std::tuple_cat(GetDataAtIndex(std::get<Datasets&>(m_datasets), index)...);
			}

			size_t GetSize()
			{
				std::vector<size_t> sizes = { GetDataSize(std::get<Datasets&>(m_datasets)) ... };
				size_t result = UINT_MAX;
				for (auto s : sizes)
				{
					if (s < result)
						result = s;
				}
				return result;
			}
		};

		template<typename  DataSet>
		class DatasetIterator
		{
			using ValueType = typename DataSet::ValueType;
			using PointerType = ValueType*;
			using ReferenceType = ValueType&;

			PointerType m_Ptr;

		public:
			DatasetIterator(PointerType ptr) : m_Ptr(ptr)
			{}

			ReferenceType operator*()
			{
				return *m_Ptr;
			}

			PointerType operator->()
			{
				return m_Ptr;
			}

			std::tuple<ReferenceType> GetTupleReference()
			{
				return { *m_Ptr };
			}

			_NODISCARD std::tuple<ReferenceType> GetTupleReference(const size_t _Off) const noexcept {
				return { (*this)[_Off] };
			}

			bool operator==(const DatasetIterator& other) const
			{
				return m_Ptr == other.m_Ptr;
			}

			bool operator!=(const DatasetIterator& other) const
			{
				return !(*this == other);
			}

			DatasetIterator& operator++() noexcept
			{
				++m_Ptr;
				return *this;
			}

			DatasetIterator operator++(int) noexcept
			{
				DatasetIterator tmp = *this;
				++* this;
				return tmp;
			}

			DatasetIterator& operator--() noexcept {
				--m_Ptr;
				return *this;
			}

			DatasetIterator operator--(int) noexcept {
				DatasetIterator tmp = *this;
				--* this;
				return tmp;
			}

			DatasetIterator& operator+=(const size_t _Off) noexcept {
				m_Ptr += _Off;
				return *this;
			}

			_NODISCARD DatasetIterator operator+(const size_t _Off) const noexcept {
				DatasetIterator tmp = *this;
				return tmp += _Off;
			}

			DatasetIterator& operator-=(const size_t _Off) noexcept {
				return *this += -_Off;
			}


			_NODISCARD DatasetIterator operator-(const size_t _Off) const noexcept {
				DatasetIterator tmp = *this;
				return tmp -= _Off;
			}

			_NODISCARD ReferenceType operator[](const size_t _Off) const noexcept {
				return *(*this + _Off);
			}
		};

		template<typename Entity, typename ComponentType>
		class DataSet : public  SparseSet<Entity> {
			std::vector<ComponentType> m_denseComponent;

			using ValueType = ComponentType;
			using Iterator = DatasetIterator<DataSet<Entity, ComponentType>>;

		public:
			explicit DataSet(size_t capacity = 512) : SparseSet<Entity>(capacity)
			{
				m_denseComponent.reserve(capacity);
			}

			FORCEINLINE void PushBack(Entity id, ComponentType& component)
			{
				if (this->m_sparse[id] != this->m_noEntity)
				{
					return; //Component already exist
				}
				size_t pos = m_denseComponent.size();
				m_denseComponent.push_back(component);
				this->m_dense.push_back(id);
				this->m_sparse[id] = pos;
			}

			template<typename ...Args>
			FORCEINLINE void EmplaceBack(Entity id, Args&& ...args)
			{
				if (this->m_sparse[id] != this->m_noEntity)
				{
					return; //Component already exist
				}
				size_t pos = m_denseComponent.size();
				if constexpr (std::is_aggregate_v<ComponentType>) {
					m_denseComponent.push_back(ComponentType{ std::forward<Args>(args)... });
				}
				else {
					m_denseComponent.emplace_back(std::forward<Args>(args)...);
				}
				this->m_dense.push_back(id);
				this->m_sparse[id] = pos;
			}

			void RemoveComponent(Entity entity)
			{
				const auto last = m_denseComponent.back();
				std::swap(m_denseComponent.back(), m_denseComponent[this->m_sparse[entity]]);
				std::swap(this->m_sparse[last], this->m_sparse[entity]);
				m_denseComponent.pop_back();
				this->m_sparse[entity] = this->m_noEntity;
				this->m_freeEntities.push(entity);
			}

			ComponentType& GetComponent(Entity id)
			{
				return m_denseComponent[this->m_sparse[id]];
			}

			FORCEINLINE bool Exist(Entity id) const
			{
				return this->m_sparse[id] != this->m_noEntity;
			}

			FORCEINLINE void Swap(size_t index, Entity entity)
			{
				auto e1 = entity;
				auto e2 = this->m_dense[index];

				auto p1 = this->m_sparse[e1];
				this->m_sparse[e1] = index;
				this->m_sparse[e2] = p1;

				std::swap(this->m_dense[index], this->m_dense[p1]);
				std::swap(m_denseComponent[index], m_denseComponent[p1]);
			}

			FORCEINLINE std::vector<ComponentType>& GetData()
			{
				return m_denseComponent;
			}

			std::tuple<ComponentType&> GetAtPosition(size_t index)
			{
				return { m_denseComponent[index] };
			}

			size_t GetSize() const
			{
				return  m_denseComponent.size();
			}
		};
	}
}
