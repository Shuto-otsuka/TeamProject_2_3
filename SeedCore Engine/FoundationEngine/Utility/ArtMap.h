#pragma once

namespace SeedCore
{
	namespace Art
	{
		template <typename K, typename = void>
		struct KeyTraits
		{
			static_assert(std::is_trivially_copyable_v<K>, "KeyTraits is not specialized for this type, and it is not trivially copyable.");

			static DynamicArray<Uint8> to_bytes(const K& key)
			{
				DynamicArray<Uint8> _out(sizeof(K));
				std::memcpy(_out.data(), &key, sizeof(K));
				return _out;
			}
		};

		template <>
		struct KeyTraits<String>
		{
			static DynamicArray<Uint8> to_bytes(String str)
			{
				DynamicArray<Uint8> _out(str.str().size() + 1);
				std::memcpy(_out.data(), str.str().data(), str.str().size());
				_out.back() = 0;
				return _out;
			}
		};

		template <>
		struct KeyTraits<std::string>
		{
			static DynamicArray<Uint8> to_bytes(const std::string& str)
			{
				DynamicArray<Uint8> _out(str.size() + 1);
				std::memcpy(_out.data(), str.data(), str.size());
				_out.back() = 0;
				return _out;
			}
		};

		template <>
		struct KeyTraits<std::string_view>
		{
			static DynamicArray<Uint8> to_bytes(std::string_view str)
			{
				DynamicArray<Uint8> _out(str.size() + 1);
				std::memcpy(_out.data(), str.data(), str.size());
				_out.back() = 0;
				return _out;
			}
		};

		template <>
		struct KeyTraits<DynamicArray<Uint8>>
		{
			static DynamicArray<Uint8> to_bytes(const DynamicArray<Uint8>& key)
			{
				return key;
			}
		};

		template <std::unsigned_integral U>
		struct KeyTraits<U>
		{
			static DynamicArray<Uint8> to_bytes(U key)
			{
				DynamicArray<Uint8> _out(sizeof(U));
				for (Size index = sizeof(U); index-- > 0;)
				{
					_out[sizeof(U) - 1 - index] = static_cast<Uint8>((key >> (index * 8)) & 0xFF);
				}
				return _out;
			}
		};

		template <std::signed_integral S>
		struct KeyTraits<S>
		{
			static DynamicArray<Uint8> to_bytes(S key)
			{
				using U = std::make_unsigned_t<S>;
				U u = static_cast<U>(key) ^ (U(1) << (sizeof(U) * 8 - 1));
				return KeyTraits<U>::to_bytes(u);
			}
		};

		SC_CONSTEX Size MAX_PREFIX = 8;

		enum class NodeType : Uint8
		{
			Node4,

			Node16,

			Node48,

			Node256,

			Leaf
		};

		struct NodeBase
		{
			explicit NodeBase(NodeType type) : type_(type)
			{
				/// No Code
			}

			virtual ~NodeBase() = default;

			Uint32 check_prefix(std::span<const Uint8> key, Uint32 depth)const noexcept
			{
				Uint32 match = 0;
				const Uint32 limit = static_cast<Uint32>(Min<Uint32>(prefixLength_, MAX_PREFIX));
				for (; match < limit && depth + match < key.size(); ++match)
				{
					if (prefix_[match] != key[depth + match])
					{
						break;
					}
				}
				return match;
			}

			NodeType type_;

			Uint8 prefixLength_ = 0;

			Uint8 prefix_[MAX_PREFIX] = {};

			Uint16 numberChildren_ = 0;
		};

		template <typename K, typename V>
		struct Leaf final : NodeBase
		{
			Leaf(K key, V value) : NodeBase(NodeType::Leaf), pair_(std::move(key), std::move(value))
			{
				/// No Code
			}

			template <typename Traits>
			Bool matches(std::span<const Uint8> key)const noexcept
			{
				return std::ranges::equal(Traits::to_bytes(pair_.first), key);
			}

			std::pair<const K, V> pair_;
		};

		struct Node4;
		struct Node16;
		struct Node48;
		struct Node256;

		using NodePtr = NodeBase*;

		struct Node4 final : NodeBase
		{
			Node4() : NodeBase(NodeType::Node4)
			{
				/// No Code
			}

			NodePtr* find(Uint8 byte)noexcept
			{
				for (Uint16 index = 0; index < numberChildren_; ++index)
				{
					if (keys_[index] == byte)
					{
						return &children_[index];
					}
				}
				return nullptr;
			}

			void add(Uint8 byte, NodePtr child)
			{
				if (numberChildren_ >= 4)
				{
					return;
				}
				Uint8 index = static_cast<Uint8>(numberChildren_);
				while (index > 0 && keys_[index - 1] > byte)
				{
					keys_[index] = keys_[index - 1];
					children_[index] = children_[index - 1];
					--index;
				}
				keys_[index] = byte;
				children_[index] = child;
				++numberChildren_;
			}

			void remove(Uint8 byte)
			{
				for (Uint16 index = 0; index < numberChildren_; ++index)
				{
					if (keys_[index] == byte)
					{
						std::copy(keys_.begin() + index + 1, keys_.begin() + numberChildren_, keys_.begin() + index);
						std::copy(children_.begin() + index + 1, children_.begin() + numberChildren_, children_.begin() + index);
						--numberChildren_;
						return;
					}
				}
			}

			NodePtr first()noexcept
			{
				return numberChildren_ ? children_[0] : nullptr;
			}

			NodePtr last()noexcept
			{
				return numberChildren_ ? children_[numberChildren_ - 1] : nullptr;
			}

			StaticArray<Uint8, 4> keys_{};

			StaticArray<NodePtr, 4> children_{};
		};

		struct Node16 final : NodeBase
		{
			Node16() : NodeBase(NodeType::Node16)
			{
				/// No Code
			}

			NodePtr* find(Uint8 byte)noexcept
			{
				for (Uint16 index = 0; index < numberChildren_; ++index)
				{
					if (keys_[index] == byte)
					{
						return &children_[index];
					}
				}
				return nullptr;
			}

			void add(Uint8 byte, NodePtr child)
			{
				if (numberChildren_ >= 16)
				{
					return;
				}
				Uint16 index = numberChildren_;
				while (index > 0 && keys_[index - 1] > byte)
				{
					keys_[index] = keys_[index - 1];
					children_[index] = children_[index - 1];
					--index;
				}
				keys_[index] = byte;
				children_[index] = child;
				++numberChildren_;
			}

			void remove(Uint8 byte)
			{
				for (Uint16 index = 0; index < numberChildren_; ++index)
				{
					if (keys_[index] == byte)
					{
						std::copy(keys_.begin() + index + 1, keys_.begin() + numberChildren_, keys_.begin() + index);
						std::copy(children_.begin() + index + 1, children_.begin() + numberChildren_, children_.begin() + index);
						--numberChildren_;
						return;
					}
				}
			}

			StaticArray<Uint8, 16> keys_{};

			StaticArray<NodePtr, 16> children_{};
		};

		struct Node48 final : NodeBase
		{
			Node48() : NodeBase(NodeType::Node48)
			{
				index_.fill(0xFF);
			}

			NodePtr* find(Uint8 byte)noexcept
			{
				if (index_[byte] == 0xFF)
				{
					return nullptr;
				}
				return &children_[index_[byte]];
			}

			void add(Uint8 byte, NodePtr child)
			{
				if (numberChildren_ >= 48)
				{
					return;
				}

				Uint8 slot = 0;
				while (slot < 48 && children_[slot] != nullptr)
				{
					++slot;
				}
				if (slot >= 48)
				{
					return;
				}
				index_[byte] = slot;
				children_[slot] = child;
				++numberChildren_;
			}

			void remove(Uint8 byte)
			{
				Uint8 slot = index_[byte];
				children_[slot] = nullptr;
				index_[byte] = 0xFF;
				--numberChildren_;
			}

			StaticArray<Uint8, 256> index_;

			StaticArray<NodePtr, 48>  children_{};
		};

		struct Node256 final : NodeBase
		{
			Node256() : NodeBase(NodeType::Node256)
			{
				/// No Code
			}

			NodePtr* find(Uint8 byte)noexcept
			{
				if (!children_[byte])
				{
					return nullptr;
				}
				return &children_[byte];
			}

			void add(Uint8 byte, NodePtr child)
			{
				children_[byte] = child;
				++numberChildren_;
			}

			void remove(Uint8 byte)
			{
				children_[byte] = nullptr;
				--numberChildren_;
			}

			StaticArray<NodePtr, 256> children_{};
		};

		inline Node16* grow(Node4* node)
		{
			auto* node16 = new Node16();
			node16->prefixLength_ = node->prefixLength_;
			node16->numberChildren_ = node->numberChildren_;
			std::memcpy(node16->prefix_, node->prefix_, node->prefixLength_);
			std::copy(node->keys_.begin(), node->keys_.begin() + node->numberChildren_, node16->keys_.begin());
			std::copy(node->children_.begin(), node->children_.begin() + node->numberChildren_, node16->children_.begin());
			return node16;
		}

		inline Node48* grow(Node16* node)
		{
			auto* node48 = new Node48();
			node48->prefixLength_ = node->prefixLength_;
			node48->numberChildren_ = node->numberChildren_;
			std::memcpy(node48->prefix_, node->prefix_, node->prefixLength_);
			for (Uint16 index = 0; index < node->numberChildren_; ++index)
			{
				node48->add(node->keys_[index], node->children_[index]);
			}
			node48->numberChildren_ = node->numberChildren_;
			return node48;
		}

		inline Node256* grow(Node48* node)
		{
			auto* node256 = new Node256();
			node256->prefixLength_ = node->prefixLength_;
			node256->numberChildren_ = node->numberChildren_;
			std::memcpy(node256->prefix_, node->prefix_, node->prefixLength_);
			for (Int b = 0; b < 256; ++b)
			{
				if (node->index_[b] != 0xFF)
				{
					node256->children_[b] = node->children_[node->index_[b]];
				}
			}
			return node256;
		}

		inline Node4* shrink(Node16* node)
		{
			auto* node4 = new Node4();
			node4->prefixLength_ = node->prefixLength_;
			node4->numberChildren_ = node->numberChildren_;
			std::memcpy(node4->prefix_, node->prefix_, node->prefixLength_);
			std::copy(node->keys_.begin(), node->keys_.begin() + node->numberChildren_, node4->keys_.begin());
			std::copy(node->children_.begin(), node->children_.begin() + node->numberChildren_, node4->children_.begin());
			return node4;
		}

		inline Node16* shrink(Node48* node)
		{
			auto* node16 = new Node16();
			node16->prefixLength_ = node->prefixLength_;
			std::memcpy(node16->prefix_, node->prefix_, node->prefixLength_);
			for (Int b = 0; b < 256; ++b)
			{
				if (node->index_[b] != 0xFF)
				{
					node16->add(static_cast<Uint8>(b), node->children_[node->index_[b]]);
				}
			}
			return node16;
		}

		inline Node48* shrink(Node256* node)
		{
			auto* node48 = new Node48();
			node48->prefixLength_ = node->prefixLength_;
			std::memcpy(node48->prefix_, node->prefix_, node->prefixLength_);
			for (Int b = 0; b < 256; ++b)
			{
				if (node->children_[b])
				{
					node48->add(static_cast<Uint8>(b), node->children_[b]);
				}
			}
			return node48;
		}

		template <typename K, typename V>
		Leaf<K, V>* as_leaf(NodePtr ptr)
		{
			return static_cast<Leaf<K, V>*>(ptr);
		}

		inline NodePtr* find(NodePtr node, Uint8 byte)
		{
			switch (node->type_)
			{
			case NodeType::Node4:
				return static_cast<Node4*>(node)->find(byte);
			case NodeType::Node16:
				return static_cast<Node16*>(node)->find(byte);
			case NodeType::Node48:
				return static_cast<Node48*>(node)->find(byte);
			case NodeType::Node256:
				return static_cast<Node256*>(node)->find(byte);
			default:
				return nullptr;
			}
		}

		inline void add(NodePtr& nodeRef, Uint8 byte, NodePtr child)
		{
			switch (nodeRef->type_)
			{
			case NodeType::Node4:
			{
				auto* node4 = static_cast<Node4*>(nodeRef);
				if (node4->numberChildren_ < 4)
				{
					node4->add(byte, child);
					return;
				}
				auto* node16 = grow(node4);
				delete node4;
				nodeRef = node16;
				node16->add(byte, child);
				return;
			}
			case NodeType::Node16:
			{
				auto* node16 = static_cast<Node16*>(nodeRef);
				if (node16->numberChildren_ < 16)
				{
					node16->add(byte, child);
					return;
				}
				auto* node48 = grow(node16);
				delete node16;
				nodeRef = node48;
				node48->add(byte, child);
				return;
			}
			case NodeType::Node48:
			{
				auto* node48 = static_cast<Node48*>(nodeRef);
				if (node48->numberChildren_ < 48)
				{
					node48->add(byte, child);
					return;
				}
				auto* node256 = grow(node48);
				delete node48;
				nodeRef = node256;
				node256->add(byte, child);
				return;
			}
			case NodeType::Node256:
				static_cast<Node256*>(nodeRef)->add(byte, child);
				return;
			default:
				break;
			}
		}

		inline void remove(NodePtr& nodeRef, Uint8 byte)
		{
			switch (nodeRef->type_)
			{
			case NodeType::Node4:
				static_cast<Node4*>(nodeRef)->remove(byte);
				break;
			case NodeType::Node16:
			{
				auto* node16 = static_cast<Node16*>(nodeRef);
				node16->remove(byte);

				if (node16->numberChildren_ <= 3)
				{
					auto* node4 = shrink(node16);
					delete node16;
					nodeRef = node4;
				}
				break;
			}
			case NodeType::Node48:
			{
				auto* node48 = static_cast<Node48*>(nodeRef);
				node48->remove(byte);
				if (node48->numberChildren_ <= 12)
				{
					auto* node16 = shrink(node48);
					delete node48;
					nodeRef = node16;
				}
				break;
			}
			case NodeType::Node256:
			{
				auto* node256 = static_cast<Node256*>(nodeRef);
				node256->remove(byte);
				if (node256->numberChildren_ <= 37)
				{
					auto* node48 = shrink(node256);
					delete node256;
					nodeRef = node48;
				}
				break;
			}
			default:
				break;
			}
		}

		template <typename K, typename V>
		Leaf<K, V>* minimum(NodePtr node)
		{
			if (!node)
			{
				return nullptr;
			}

			if (node->type_ == NodeType::Leaf)
			{
				return as_leaf<K, V>(node);
			}

			switch (node->type_)
			{
			case NodeType::Node4:
				return minimum<K, V>(static_cast<Node4*>(node)->first());
			case NodeType::Node16:
				return minimum<K, V>(static_cast<Node16*>(node)->children_[0]);
			case NodeType::Node48:
			{
				auto* node48 = static_cast<Node48*>(node);
				for (Int b = 0; b < 256; ++b)
				{
					if (node48->index_[b] != 0xFF)
					{
						return minimum<K, V>(node48->children_[node48->index_[b]]);
					}
				}
				return nullptr;
			}
			case NodeType::Node256:
			{
				auto* node256 = static_cast<Node256*>(node);
				for (Int b = 0; b < 256; ++b)
				{
					if (node256->children_[b])
					{
						return minimum<K, V>(node256->children_[b]);
					}
				}
				return nullptr;
			}
			default:
				return nullptr;
			}
		}

		template <typename K, typename V>
		void destroy(NodePtr node)
		{
			if (!node)
			{
				return;
			}

			if (node->type_ == NodeType::Leaf)
			{
				delete as_leaf<K, V>(node);
				return;
			}

			switch (node->type_)
			{
			case NodeType::Node4:
			{
				auto* node4 = static_cast<Node4*>(node);
				for (Uint16 index = 0; index < node4->numberChildren_; ++index)
				{
					destroy<K, V>(node4->children_[index]);
				}
				delete node4;
				break;
			}
			case NodeType::Node16:
			{
				auto* node16 = static_cast<Node16*>(node);
				for (Uint16 index = 0; index < node16->numberChildren_; ++index)
				{
					destroy<K, V>(node16->children_[index]);
				}
				delete node16;
				break;
			}
			case NodeType::Node48:
			{
				auto* node48 = static_cast<Node48*>(node);
				for (Int b = 0; b < 256; ++b)
				{
					if (node48->index_[b] != 0xFF)
					{
						destroy<K, V>(node48->children_[node48->index_[b]]);
					}
				}
				delete node48;
				break;
			}
			case NodeType::Node256:
			{
				auto* node256 = static_cast<Node256*>(node);
				for (Int b = 0; b < 256; ++b)
				{
					destroy<K, V>(node256->children_[b]);
				}
				delete node256;
				break;
			}
			default:
				break;
			}
		}

		template <typename K, typename V, typename F>
		void traverse(NodePtr node, F&& function)
		{
			if (!node)
			{
				return;
			}

			if (node->type_ == NodeType::Leaf)
			{
				function(*as_leaf<K, V>(node));
				return;
			}

			switch (node->type_)
			{
			case NodeType::Node4:
			{
				auto* node4 = static_cast<Node4*>(node);
				for (Uint16 index = 0; index < node4->numberChildren_; ++index)
				{
					traverse<K, V>(node4->children_[index], function);
				}
				break;
			}
			case NodeType::Node16:
			{
				auto* node16 = static_cast<Node16*>(node);
				for (Uint16 index = 0; index < node16->numberChildren_; ++index)
				{
					traverse<K, V>(node16->children_[index], function);
				}
				break;
			}
			case NodeType::Node48:
			{
				auto* node48 = static_cast<Node48*>(node);
				for (Int b = 0; b < 256; ++b)
				{
					if (node48->index_[b] != 0xFF)
					{
						traverse<K, V>(node48->children_[node48->index_[b]], function);
					}
				}
				break;
			}
			case NodeType::Node256:
			{
				auto* node256 = static_cast<Node256*>(node);
				for (Int b = 0; b < 256; ++b)
				{
					traverse<K, V>(node256->children_[b], function);
				}
				break;
			}
			default:
				break;
			}
		}

		struct StackFrame
		{
			NodePtr node_ = nullptr;

			Int cursor_ = 0;
		};

		template <typename K, typename V>
		Leaf<K, V>* descend(DynamicArray<StackFrame>& stack)
		{
			while (!stack.empty())
			{
				auto& [node, cursor] = stack.back();

				if (node->type_ == NodeType::Leaf)
				{
					return as_leaf<K, V>(node);
				}

				NodePtr next = nullptr;

				switch (node->type_)
				{
				case NodeType::Node4:
				{
					auto* node4 = static_cast<Node4*>(node);
					if (cursor < node4->numberChildren_)
					{
						next = node4->children_[cursor++];
					}
					break;
				}
				case NodeType::Node16:
				{
					auto* node16 = static_cast<Node16*>(node);
					if (cursor < node16->numberChildren_)
					{
						next = node16->children_[cursor++];
					}
					break;
				}
				case NodeType::Node48:
				{
					auto* node48 = static_cast<Node48*>(node);
					while (cursor < 256 && node48->index_[cursor] == 0xFF)
					{
						++cursor;
					}
					if (cursor < 256)
					{
						next = node48->children_[node48->index_[cursor++]];
					}
					break;
				}
				case NodeType::Node256:
				{
					auto* node256 = static_cast<Node256*>(node);
					while (cursor < 256 && !node256->children_[cursor])
					{
						++cursor;
					}
					if (cursor < 256)
					{
						next = node256->children_[cursor++];
					}
					break;
				}
				default:
					break;
				}

				if (next)
				{
					stack.push_back({ next, 0 });
				}
				else
				{
					stack.pop_back();
				}
			}
			return nullptr;
		}
	}
}

namespace SeedCore
{
	template <typename K, typename V, typename Traits = Art::KeyTraits<K>>
	class ArtMap
	{
	private:
		using Leaf = Art::Leaf<K, V>;
		using NodePtr = Art::NodePtr;

		using KeyType = DynamicArray<Uint8>;
		using MappedType = V;
		using ValueType = std::pair<const K, MappedType>;

	public:
		struct Iterator
		{
			using iterator_category = std::forward_iterator_tag;
			using value_type = std::pair<const K, V>;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using reference = value_type&;

			Iterator() = default;

			explicit Iterator(NodePtr root)
			{
				if (root)
				{
					stack_.push_back({ root, 0 });
					advance();
				}
			}

			reference operator*()const
			{
				return current_->pair_;
			}

			pointer operator->()const
			{
				return &current_->pair_;
			}

			Iterator& operator++()
			{
				stack_.pop_back();
				advance();
				return *this;
			}

			Iterator operator++(Int)
			{
				Iterator temporary = *this;
				++(*this);
				return temporary;
			}

			Bool operator==(const Iterator& other)const noexcept
			{
				return current_ == other.current_;
			}

			Bool operator!=(const Iterator& other)const noexcept
			{
				return !(*this == other);
			}

		private:
			DynamicArray<Art::StackFrame> stack_;

			Leaf* current_ = nullptr;

			void advance()
			{
				current_ = Art::descend<K, V>(stack_);
			}
		};

		struct ConstIterator
		{
			using iterator_category = std::forward_iterator_tag;
			using value_type = std::pair<const K, V>;
			using difference_type = std::ptrdiff_t;
			using pointer = const value_type*;
			using reference = const value_type&;

			ConstIterator() = default;

			explicit ConstIterator(NodePtr root)
			{
				if (root)
				{
					stack_.push_back({ root, 0 });
					advance();
				}
			}

			ConstIterator(const Iterator& it) : stack_(it.stack_), current_(it.current_)
			{
				/// No Code
			}

			reference operator*()const
			{
				return  current_->pair_;
			}

			pointer operator->()const
			{
				return &current_->pair_;
			}

			ConstIterator& operator++()
			{
				stack_.pop_back();
				advance();
				return *this;
			}

			ConstIterator operator++(Int)
			{
				ConstIterator temporary = *this;
				++(*this);
				return temporary;
			}

			Bool operator==(const ConstIterator& other)const noexcept
			{
				return current_ == other.current_;
			}

			Bool operator!=(const ConstIterator& other)const noexcept
			{
				return !(*this == other);
			}

		private:
			DynamicArray<Art::StackFrame> stack_;

			Leaf* current_ = nullptr;

			void advance()
			{
				current_ = Art::descend<K, V>(stack_);
			}
		};

	public:
		ArtMap() = default;

		~ArtMap()
		{
			Art::destroy<K, V>(root_);
		}

		ArtMap(const ArtMap&) = delete;
		ArtMap& operator=(const ArtMap&) = delete;

		ArtMap(ArtMap&& o)noexcept : root_(o.root_), size_(o.size_)
		{
			o.root_ = nullptr;
			o.size_ = 0;
		}

		ArtMap(std::initializer_list<std::pair<const K, V>> init) :root_(nullptr), size_(0)
		{
			for (const auto& pair : init)
			{
				insert(pair.first, pair.second);
			}
		}

		ArtMap& operator=(ArtMap&& o)noexcept
		{
			if (this != &o)
			{
				Art::destroy<K, V>(root_);
				root_ = o.root_;
				size_ = o.size_;
				o.root_ = nullptr;
				o.size_ = 0;
			}
			return *this;
		}

		ArtMap& operator=(std::initializer_list<std::pair<const K, V>> init)
		{
			clear();
			for (const auto& pair : init)
			{
				insert(pair.first, pair.second);
			}
			return *this;
		}

		Bool insert(const K& key, V value)
		{
			auto keyBite = Traits::to_bytes(key);
			return insert_implementation(root_, key, keyBite, std::move(value), 0);
		}

		void upsert(const K& key, V value)
		{
			auto keyBite = Traits::to_bytes(key);
			upsert_implementation(root_, key, keyBite, std::move(value), 0);
		}

		template<class F>
		void prefix_search(const K& prefix, F&& callback)const
		{
			prefix_search_implementation(prefix, std::forward<F>(callback));
		}

		DynamicArray<const ValueType*> prefix_search(const K& prefix)const
		{
			DynamicArray<const ValueType*> result;

			prefix_search_implementation(prefix, [&](const Leaf* leaf) {result.push_back(&leaf->pair_);});

			return result;
		}

		std::optional<V> find(const K& key)const
		{
			auto keyBite = Traits::to_bytes(key);
			Leaf* leaf = find_leaf(keyBite);
			if (!leaf)
			{
				return std::nullopt;
			}
			return leaf->pair_.second;
		}

		V& at(const K& key)
		{
			auto keyBite = Traits::to_bytes(key);
			Leaf* leaf = find_leaf(keyBite);
			if (!leaf)
			{
				throw std::out_of_range("ArtMap::at: key not found");
			}
			return leaf->pair_.second;
		}

		const V& at(const K& key)const
		{
			auto keyBite = Traits::to_bytes(key);
			const Leaf* leaf = find_leaf(keyBite);
			if (!leaf)
			{
				throw std::out_of_range("ArtMap::at: key not found");
			}
			return leaf->pair_.second;
		}

		Bool contains(const K& key)const
		{
			auto keyBite = Traits::to_bytes(key);
			return find_leaf(keyBite) != nullptr;
		}

		Bool erase(const K& key)
		{
			auto keyBite = Traits::to_bytes(key);
			Bool removed = false;
			erase_implementation(root_, keyBite, 0, removed);
			return removed;
		}

		NODISCARD Size size()const noexcept
		{
			return size_;
		}

		NODISCARD Bool empty()const noexcept
		{
			return size_ == 0;
		}

		void clear()
		{
			Art::destroy<K, V>(root_);
			root_ = nullptr;
			size_ = 0;
		}

		Iterator begin()
		{
			return Iterator(root_);
		}

		Iterator end()
		{
			return Iterator();
		}

		ConstIterator begin()const
		{
			return ConstIterator(root_);
		}

		ConstIterator end()const
		{
			return ConstIterator();
		}

		ConstIterator cbegin()const
		{
			return ConstIterator(root_);
		}

		ConstIterator cend()const
		{
			return ConstIterator();
		}

	private:
		NodePtr root_ = nullptr;

		Size size_ = 0;

		Leaf* find_leaf(std::span<const Uint8> key)const
		{
			NodePtr node = root_;
			Uint32 depth = 0;

			while (node)
			{
				if (node->type_ == Art::NodeType::Leaf)
				{
					auto* leaf = Art::as_leaf<K, V>(node);
					return leaf->template matches<Traits>(key) ? leaf : nullptr;
				}

				Uint32 prefix = node->check_prefix(key, depth);
				if (prefix < node->prefixLength_)
				{
					return nullptr;
				}
				depth += node->prefixLength_;
				if (depth >= key.size())
				{
					return nullptr;
				}

				NodePtr* child = Art::find(node, key[depth]);
				if (!child)
				{
					return nullptr;
				}
				node = *child;
				++depth;
			}
			return nullptr;
		}

		NodePtr find_prefix_node(std::span<const Uint8> prefix)const
		{
			NodePtr node = root_;
			Uint32 depth = 0;

			while (node)
			{
				if (node->type_ == Art::NodeType::Leaf)
				{
					auto* leaf = Art::as_leaf<K, V>(node);
					auto bytes = Traits::to_bytes(leaf->pair_.first);

					if (bytes.size() < prefix.size())
					{
						return nullptr;
					}

					if (std::equal(prefix.begin(), prefix.end(), bytes.begin()))
					{
						return node;
					}

					return nullptr;
				}

				Uint32 matched = node->check_prefix(prefix, depth);

				if (depth + matched == prefix.size())
				{
					return node;
				}

				if (matched < node->prefixLength_)
				{
					return nullptr;
				}

				depth += node->prefixLength_;

				NodePtr* child = Art::find(node, prefix[depth]);
				if (!child)
				{
					return nullptr;
				}

				node = *child;
				++depth;

				if (depth >= prefix.size())
				{
					return node;
				}
			}

			return nullptr;
		}

		Bool insert_implementation(NodePtr& node, const K& originalKey, std::span<const Uint8> key, V value, Uint32 depth)
		{
			if (!node) 
			{
				node = new Leaf(originalKey, std::move(value));
				++size_;
				return true;
			}

			if (node->type_ == Art::NodeType::Leaf)
			{
				auto* existing = Art::as_leaf<K, V>(node);
				if (existing->template matches<Traits>(key))
				{
					return false;
				}

				auto* newNode = new Art::Node4();
				Uint32 common = 0;
				auto existingBytes = Traits::to_bytes(existing->pair_.first);
				while (depth + common < existingBytes.size() && depth + common < key.size() && existingBytes[depth + common] == key[depth + common])
				{
					++common;
				}

				const Uint32 effectiveCommon = Min<Uint32>(common, static_cast<Uint32>(Art::MAX_PREFIX));
				newNode->prefixLength_ = static_cast<Uint8>(effectiveCommon);
				std::memcpy(newNode->prefix_, existingBytes.data() + depth, effectiveCommon);

				Uint32 splitDepth = depth + effectiveCommon;

				if (common > Art::MAX_PREFIX)
				{
					Uint8 sharedByte = existingBytes[splitDepth];
					newNode->add(sharedByte, existing);
					node = newNode;
					NodePtr* slot = newNode->find(sharedByte);
					return insert_implementation(*slot, originalKey, key, std::move(value), splitDepth + 1);
				}

				Uint8 oldByte = (splitDepth < existingBytes.size()) ? existingBytes[splitDepth] : 0;
				Uint8 newByte = (splitDepth < key.size()) ? key[splitDepth] : 0;

				newNode->add(oldByte, existing);
				auto* newLeaf = new Leaf(originalKey, std::move(value));
				newNode->add(newByte, newLeaf);

				node = newNode;
				++size_;
				return true;
			}

			Uint32 prefix = node->check_prefix(key, depth);
			if (prefix < node->prefixLength_) 
			{
				auto* newNode = new Art::Node4();
				newNode->prefixLength_ = static_cast<Uint8>(prefix);
				std::memcpy(newNode->prefix_, node->prefix_, prefix);

				auto* anyLeaf = Art::minimum<K, V>(node);
				auto anyBytes = Traits::to_bytes(anyLeaf->pair_.first);
				Uint8 oldByte = node->prefix_[prefix];
				newNode->add(oldByte, node);

				node->prefixLength_ -= (prefix + 1);
				std::memmove(node->prefix_, anyBytes.data() + depth + prefix + 1, Min<Uint32>(node->prefixLength_, Art::MAX_PREFIX));

				Uint8 newByte = (depth + prefix < key.size()) ? key[depth + prefix] : 0;
				auto* newLeaf = new Leaf(originalKey, std::move(value));
				newNode->add(newByte, newLeaf);

				node = newNode;
				++size_;
				return true;
			}

			depth += node->prefixLength_;
			if (depth >= key.size())
			{
				return false;
			}

			NodePtr* child = Art::find(node, key[depth]);
			if (child) 
			{
				return insert_implementation(*child, originalKey, key, std::move(value), depth + 1);
			}
			auto* newLeaf = new Leaf(originalKey, std::move(value));
			Art::add(node, key[depth], newLeaf);
			++size_;
			return true;
		}

		void upsert_implementation(NodePtr& node, const K& originalKey, std::span<const Uint8> key, V value, Uint32 depth)
		{
			if (!node) 
			{
				node = new Leaf(originalKey, std::move(value));
				++size_;
				return;
			}

			if (node->type_ == Art::NodeType::Leaf)
			{
				auto* existing = Art::as_leaf<K, V>(node);
				if (existing->template matches<Traits>(key))
				{
					existing->pair_.second = std::move(value);
					return;
				}

				Bool added = insert_implementation(node, originalKey, key, std::move(value), depth);
				(void)added;
				return;
			}

			Uint32 prefix = node->check_prefix(key, depth);
			if (prefix < node->prefixLength_) 
			{
				insert_implementation(node, originalKey, key, std::move(value), depth);
				return;
			}
			depth += node->prefixLength_;
			if (depth >= key.size())
			{
				return;
			}

			NodePtr* child = Art::find(node, key[depth]);
			if (child) 
			{
				upsert_implementation(*child, originalKey, key, std::move(value), depth + 1);
				return;
			}

			auto* newLeaf = new Leaf(originalKey, std::move(value));
			Art::add(node, key[depth], newLeaf);
			++size_;
		}

		void erase_implementation(NodePtr& node, std::span<const Uint8> key, Uint32 depth, Bool& removed)
		{
			if (!node)
			{
				return;
			}

			if (node->type_ == Art::NodeType::Leaf)
			{
				auto* leaf = Art::as_leaf<K, V>(node);
				if (leaf->template matches<Traits>(key))
				{
					delete leaf;
					node = nullptr;
					--size_;
					removed = true;
				}
				return;
			}

			Uint32 prefix = node->check_prefix(key, depth);
			if (prefix < node->prefixLength_)
			{
				return;
			}
			depth += node->prefixLength_;
			if (depth >= key.size())
			{
				return;
			}

			NodePtr* child = Art::find(node, key[depth]);
			if (!child)
			{
				return;
			}

			if ((*child)->type_ == Art::NodeType::Leaf) 
			{
				auto* leaf = Art::as_leaf<K, V>(*child);
				if (leaf->template matches<Traits>(key))
				{
					delete leaf;
					*child = nullptr;
					Art::remove(node, key[depth]);
					--size_;
					removed = true;
				}
			}
			else
			{
				erase_implementation(*child, key, depth + 1, removed);
			}

			if (removed && node)
			{
				if (node->numberChildren_ == 1)
				{
					NodePtr onlyChild = nullptr;
					Uint8 onlyChildKeyByte = 0;

					switch (node->type_)
					{
					case Art::NodeType::Node4:
					{
						auto* node4 = static_cast<Art::Node4*>(node);
						for (Uint32 index = 0; index < 4; ++index)
						{
							if (node4->children_[index] != nullptr)
							{
								onlyChild = node4->children_[index];
								onlyChildKeyByte = node4->keys_[index];
								break;
							}
						}
						break;
					}
					case Art::NodeType::Node16:
					{
						auto* node16 = static_cast<Art::Node16*>(node);
						for (Uint32 index = 0; index < 16; ++index)
						{
							if (node16->children_[index] != nullptr)
							{
								onlyChild = node16->children_[index];
								onlyChildKeyByte = node16->keys_[index];
								break;
							}
						}
						break;
					}
					case Art::NodeType::Node48:
					{
						auto* node48 = static_cast<Art::Node48*>(node);
						for (Uint32 index = 0; index < 256; ++index)
						{
							if (node48->index_[index] != 0xFF)
							{
								onlyChild = node48->children_[node48->index_[index]];
								onlyChildKeyByte = static_cast<Uint8>(index);
								break;
							}
						}
						break;
					}
					case Art::NodeType::Node256:
					{
						auto* node256 = static_cast<Art::Node256*>(node);
						for (Uint32 index = 0; index < 256; ++index)
						{
							if (node256->children_[index] != nullptr)
							{
								onlyChild = node256->children_[index];
								onlyChildKeyByte = static_cast<Uint8>(index);
								break;
							}
						}
						break;
					}
					default:
						break;
					}

					if (onlyChild != nullptr)
					{
						if (onlyChild->type_ != Art::NodeType::Leaf)
						{
							Uint32 newPrefixLength = node->prefixLength_ + 1 + onlyChild->prefixLength_;

							if (newPrefixLength <= Art::MAX_PREFIX)
							{
								Uint8 temporaryPrefix[Art::MAX_PREFIX] = { 0 };
								Uint32 cacheIndex = 0;

								for (Uint32 index = 0; index < node->prefixLength_; ++index)
								{
									temporaryPrefix[cacheIndex++] = node->prefix_[index];
								}
								temporaryPrefix[cacheIndex++] = onlyChildKeyByte;
								for (Uint32 index = 0; index < onlyChild->prefixLength_; ++index)
								{
									temporaryPrefix[cacheIndex++] = onlyChild->prefix_[index];
								}

								onlyChild->prefixLength_ = newPrefixLength;
								for (Uint32 index = 0; index < newPrefixLength; ++index)
								{
									onlyChild->prefix_[index] = temporaryPrefix[index];
								}

								NodePtr oldNode = node;
								node = onlyChild;
								delete oldNode;
							}
						}
						else
						{
							NodePtr oldNode = node;
							node = onlyChild;
							delete oldNode;
						}
					}
				}
			}
		}

		template<class F>
		void prefix_search_implementation(const K& prefix, F&& callback)const
		{
			auto prefixBytes = Traits::to_bytes(prefix);

			NodePtr node = find_prefix_node(prefixBytes);

			if (!node)
			{
				return;
			}

			DynamicArray<Art::StackFrame> stack;
			stack.push_back({ node, 0 });

			while (auto* leaf = Art::descend<K, V>(stack))
			{
				callback(leaf);
				stack.pop_back();
			}
		}
	};
}
