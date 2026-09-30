using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;

namespace SeedCore
{
	[Generator(LanguageNames.CSharp)]
	public sealed class ScriptGenerator : IIncrementalGenerator
	{
		private const Int32 blockCapacity_ = 256;

		private static readonly DiagnosticDescriptor notPublic_ = new DiagnosticDescriptor("SEED001", "リフレクション属性は public フィールドにだけ付けられます", "{0} は public ではないため、インスペクターにもシーンにも出ません。public にしてください", "SeedCore", DiagnosticSeverity.Error, true);

		private static readonly DiagnosticDescriptor fieldAndRange_ = new DiagnosticDescriptor("SEED002", "SeedReflectionField / SeedReflectionRange / SeedPayloadField は併用できません", "{0} に SeedReflectionField / SeedReflectionRange / SeedPayloadField のうち複数が付いています。どれか 1 つにしてください", "SeedCore", DiagnosticSeverity.Error, true);

		private static readonly DiagnosticDescriptor conditionOnly_ = new DiagnosticDescriptor("SEED003", "SeedReflectionFieldCondition だけでは使えません", "{0} の SeedReflectionFieldCondition には、SeedReflectionField / SeedReflectionRange / SeedPayloadField のどれかが必要です", "SeedCore", DiagnosticSeverity.Error, true);

		private static readonly DiagnosticDescriptor unsupported_ = new DiagnosticDescriptor("SEED004", "このフィールドはリフレクションできません", "{0} は {1} ため、リフレクション属性を付けられません", "SeedCore", DiagnosticSeverity.Error, true);

		private static readonly DiagnosticDescriptor conditionSyntax_ = new DiagnosticDescriptor("SEED005", "表示条件の式が正しくありません", "{0} の表示条件 \"{1}\" を式として読めません", "SeedCore", DiagnosticSeverity.Error, true);

		private static readonly DiagnosticDescriptor conditionMember_ = new DiagnosticDescriptor("SEED006", "表示条件は public なメンバーだけを使えます", "{0} の表示条件で使っている {1} は public ではありません", "SeedCore", DiagnosticSeverity.Error, true);

		private static readonly DiagnosticDescriptor blockOverflow_ = new DiagnosticDescriptor("SEED007", "リフレクションするフィールドが多すぎます", "{0} のフィールドの合計が {1} バイトで、上限の {2} バイトを超えています", "SeedCore", DiagnosticSeverity.Error, true);

		public void Initialize(IncrementalGeneratorInitializationContext context)
		{
			Func<SyntaxNode, CancellationToken, Boolean> isCandidate = (node, cancellation) => node is ClassDeclarationSyntax declaration && declaration.BaseList != null;
			Func<GeneratorSyntaxContext, CancellationToken, INamedTypeSymbol> toSymbol = (syntaxContext, cancellation) => syntaxContext.SemanticModel.GetDeclaredSymbol((ClassDeclarationSyntax)syntaxContext.Node, cancellation) as INamedTypeSymbol;
			Func<INamedTypeSymbol, Boolean> isScript = symbol =>
			{
				if (symbol == null || symbol.IsAbstract || symbol.IsGenericType)
				{
					return false;
				}

				for (INamedTypeSymbol baseType = symbol.BaseType; baseType != null; baseType = baseType.BaseType)
				{
					if (baseType.ToDisplayString() == "SeedCore.SeedScript")
					{
						return true;
					}
				}
				return false;
			};

			IncrementalValuesProvider<INamedTypeSymbol> candidates = context.SyntaxProvider.CreateSyntaxProvider(isCandidate, toSymbol);
			IncrementalValuesProvider<INamedTypeSymbol> scripts = candidates.Where(isScript);

			context.RegisterSourceOutput(scripts.Collect(), Generate);
		}

		private static void Generate(SourceProductionContext context, ImmutableArray<INamedTypeSymbol> scripts)
		{
			Func<IFieldSymbol, String, AttributeData> findAttribute = (field, attributeName) => field.GetAttributes().FirstOrDefault(attribute => attribute.AttributeClass?.ToDisplayString() == attributeName);
			Func<Double, String> doubleLiteral = value =>
			{
				if (Double.IsNegativeInfinity(value))
				{
					return "global::System.Double.NegativeInfinity";
				}
				if (Double.IsPositiveInfinity(value))
				{
					return "global::System.Double.PositiveInfinity";
				}
				return value.ToString("R", CultureInfo.InvariantCulture) + "d";
			};

			StringBuilder registers = new StringBuilder();
			StringBuilder methods = new StringBuilder();

			HashSet<INamedTypeSymbol> generated = new HashSet<INamedTypeSymbol>(SymbolEqualityComparer.Default);
			Int32 scriptIndex = 0;
			foreach (INamedTypeSymbol script in scripts)
			{
				if (!generated.Add(script))
				{
					continue;
				}

				Dictionary<String, ISymbol> members = new Dictionary<String, ISymbol>();
				for (INamedTypeSymbol type = script; type != null && type.ToDisplayString() != "SeedCore.SeedScript"; type = type.BaseType)
				{
					foreach (ISymbol member in type.GetMembers())
					{
						if (!member.IsImplicitlyDeclared && !members.ContainsKey(member.Name))
						{
							members.Add(member.Name, member);
						}
					}
				}

				String scriptName = script.ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat);
				StringBuilder fieldList = new StringBuilder();
				StringBuilder push = new StringBuilder();
				StringBuilder pull = new StringBuilder();
				StringBuilder conditions = new StringBuilder();
				Int32 offset = 0;
				Int32 fieldIndex = 0;
				Int32 stringIndex = 0;

				foreach (IFieldSymbol field in script.GetMembers().OfType<IFieldSymbol>())
				{
					AttributeData reflection = findAttribute(field, "SeedCore.SeedReflectionFieldAttribute");
					AttributeData range = findAttribute(field, "SeedCore.SeedReflectionRangeAttribute");
					AttributeData payload = findAttribute(field, "SeedCore.SeedPayloadFieldAttribute");
					AttributeData condition = findAttribute(field, "SeedCore.SeedReflectionFieldConditionAttribute");
					AttributeData serialize = findAttribute(field, "SeedCore.SeedSerializeFieldAttribute");
					if (reflection == null && range == null && payload == null && condition == null && serialize == null)
					{
						continue;
					}

					Int32 displayAttributeCount = (reflection != null ? 1 : 0) + (range != null ? 1 : 0) + (payload != null ? 1 : 0);
					Location location = field.Locations.FirstOrDefault();
					if (field.DeclaredAccessibility != Accessibility.Public)
					{
						context.ReportDiagnostic(Diagnostic.Create(notPublic_, location, field.Name));
						continue;
					}
					if (displayAttributeCount > 1)
					{
						context.ReportDiagnostic(Diagnostic.Create(fieldAndRange_, location, field.Name));
						continue;
					}
					if (condition != null && displayAttributeCount == 0)
					{
						context.ReportDiagnostic(Diagnostic.Create(conditionOnly_, location, field.Name));
						continue;
					}
					if (field.IsStatic || field.IsConst || field.IsReadOnly)
					{
						context.ReportDiagnostic(Diagnostic.Create(unsupported_, location, field.Name, "static / const / readonly の"));
						continue;
					}

					String fieldTypeName = field.Type.ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat);
					ValueKind value = Classify(field.Type);
					if (value.kind_ == null)
					{
						context.ReportDiagnostic(Diagnostic.Create(unsupported_, location, field.Name, value.unsupportedReason_ ?? $"未対応の型（{field.Type.ToDisplayString()}）の"));
						continue;
					}
					if (payload != null && field.Type.SpecialType != SpecialType.System_Int32 && field.Type.SpecialType != SpecialType.System_UInt32)
					{
						context.ReportDiagnostic(Diagnostic.Create(unsupported_, location, field.Name, "SeedPayloadField は Int32 / UInt32 にしか付けられない"));
						continue;
					}

					String kind = value.kind_;
					Int32 size = value.size_;
					Int32 alignment = value.alignment_;

					String displayName = field.Name;
					Double minimum = Double.NegativeInfinity;
					Double maximum = Double.PositiveInfinity;
					String assetType = "global::SeedCore.PayloadType.None";
					if (reflection != null && reflection.ConstructorArguments.Length == 1)
					{
						displayName = (String)reflection.ConstructorArguments[0].Value;
					}
					if (range != null)
					{
						Int32 rangeStart = range.ConstructorArguments.Length == 3 ? 1 : 0;
						if (rangeStart == 1)
						{
							displayName = (String)range.ConstructorArguments[0].Value;
						}
						minimum = Convert.ToDouble(range.ConstructorArguments[rangeStart].Value, CultureInfo.InvariantCulture);
						maximum = Convert.ToDouble(range.ConstructorArguments[rangeStart + 1].Value, CultureInfo.InvariantCulture);
					}
					if (payload != null)
					{
						if (payload.ConstructorArguments.Length == 2)
						{
							displayName = (String)payload.ConstructorArguments[0].Value;
						}
						assetType = $"(global::SeedCore.PayloadType){Convert.ToInt32(payload.ConstructorArguments[payload.ConstructorArguments.Length - 1].Value, CultureInfo.InvariantCulture)}";
					}

					String conditionCode = null;
					if (condition != null)
					{
						String conditionText = (String)condition.ConstructorArguments[0].Value ?? "";
						ExpressionSyntax expression = SyntaxFactory.ParseExpression(conditionText);
						if (String.IsNullOrWhiteSpace(conditionText) || expression.GetDiagnostics().Any(diagnostic => diagnostic.Severity == DiagnosticSeverity.Error))
						{
							context.ReportDiagnostic(Diagnostic.Create(conditionSyntax_, location, field.Name, conditionText));
							continue;
						}

						ConditionRewriter rewriter = new ConditionRewriter(members);
						conditionCode = rewriter.Visit(expression).ToFullString();
						if (rewriter.nonPublic_ != null)
						{
							context.ReportDiagnostic(Diagnostic.Create(conditionMember_, location, field.Name, rewriter.nonPublic_));
							continue;
						}
					}

					offset = (offset + alignment - 1) / alignment * alignment;

					String flag = "global::SeedCore.ScriptFieldFlag.Serialize";
					if (displayAttributeCount > 0)
					{
						flag += " | global::SeedCore.ScriptFieldFlag.Reflection";
					}
					if (conditionCode != null)
					{
						flag += " | global::SeedCore.ScriptFieldFlag.Condition";
						conditions.AppendLine($"\t\t\t\tcase {fieldIndex}:");
						conditions.AppendLine($"\t\t\t\t\treturn {conditionCode};");
					}

					String enumType = kind == "Enum" ? $"typeof({fieldTypeName})" : "null";
					fieldList.AppendLine($"\t\t\t\tnew global::SeedCore.ScriptField({SymbolDisplay.FormatLiteral(displayName, true)}, global::SeedCore.AttributeType.{kind}, {offset}, {doubleLiteral(minimum)}, {doubleLiteral(maximum)}, {flag}, {enumType}, {assetType}),");

					String member = $"o.{field.Name}";
					String address = $"(block + {offset})";
					if (kind == "String")
					{
						push.AppendLine($"\t\t\t{member} = global::SeedCore.ScriptBlock.PushString(script, {stringIndex}, {address});");
						pull.AppendLine($"\t\t\tglobal::SeedCore.ScriptBlock.PullString(script, {stringIndex}, {address}, {member});");
						++stringIndex;
					}
					else
					{
						push.AppendLine($"\t\t\t{member} = {value.Read(address)};");
						pull.AppendLine($"\t\t\t{value.Write(address, member)}");
					}

					offset += size;
					++fieldIndex;
				}

				Int32 blockSize = (offset + 7) / 8 * 8;
				if (blockSize > blockCapacity_)
				{
					context.ReportDiagnostic(Diagnostic.Create(blockOverflow_, script.Locations.FirstOrDefault(), script.Name, blockSize, blockCapacity_));
					continue;
				}

				registers.AppendLine($"\t\t\tschemas[typeof({scriptName})] = new global::SeedCore.ScriptSchema(new global::SeedCore.ScriptField[]");
				registers.AppendLine("\t\t\t{");
				registers.Append(fieldList.ToString());
				registers.AppendLine($"\t\t\t}}, {blockSize}, {stringIndex}, Push{scriptIndex}, Pull{scriptIndex}, Condition{scriptIndex});");

				methods.AppendLine();
				methods.AppendLine($"\t\tprivate static void Push{scriptIndex}(global::SeedCore.SeedScript script, global::System.Byte* block)");
				methods.AppendLine("\t\t{");
				methods.AppendLine($"\t\t\t{scriptName} o = ({scriptName})script;");
				methods.Append(push.ToString());
				methods.AppendLine("\t\t}");
				methods.AppendLine();
				methods.AppendLine($"\t\tprivate static void Pull{scriptIndex}(global::SeedCore.SeedScript script, global::System.Byte* block)");
				methods.AppendLine("\t\t{");
				methods.AppendLine($"\t\t\t{scriptName} o = ({scriptName})script;");
				methods.Append(pull.ToString());
				methods.AppendLine("\t\t}");
				methods.AppendLine();
				methods.AppendLine($"\t\tprivate static global::System.Boolean Condition{scriptIndex}(global::SeedCore.SeedScript script, global::System.Int32 fieldIndex)");
				methods.AppendLine("\t\t{");
				methods.AppendLine($"\t\t\t{scriptName} o = ({scriptName})script;");
				methods.AppendLine("\t\t\tswitch (fieldIndex)");
				methods.AppendLine("\t\t\t{");
				methods.Append(conditions.ToString());
				methods.AppendLine("\t\t\t\tdefault:");
				methods.AppendLine("\t\t\t\t\treturn true;");
				methods.AppendLine("\t\t\t}");
				methods.AppendLine("\t\t}");

				++scriptIndex;
			}

			StringBuilder builder = new StringBuilder();
			builder.AppendLine("// <auto-generated/>");
			builder.AppendLine("#pragma warning disable");
			builder.AppendLine();
			builder.AppendLine("namespace SeedCore.Generated");
			builder.AppendLine("{");
			builder.AppendLine("\tinternal static unsafe class ScriptReflection");
			builder.AppendLine("\t{");
			builder.AppendLine("\t\tinternal static void Register(global::System.Collections.Generic.Dictionary<global::System.Type, global::SeedCore.ScriptSchema> schemas)");
			builder.AppendLine("\t\t{");
			builder.Append(registers.ToString());
			builder.AppendLine("\t\t}");
			builder.Append(methods.ToString());
			builder.AppendLine("\t}");
			builder.AppendLine("}");

			context.AddSource("Reflection.generated.cs", SourceText.From(builder.ToString(), Encoding.UTF8));
		}

		private static ValueKind Classify(ITypeSymbol type)
		{
			ValueKind value = new ValueKind();
			value.typeName_ = type.ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat);
			switch (type.SpecialType)
			{
			case SpecialType.System_SByte:
			case SpecialType.System_Byte:
			case SpecialType.System_Int16:
			case SpecialType.System_UInt16:
			case SpecialType.System_Int32:
			case SpecialType.System_UInt32:
				value.kind_ = "Int";
				break;
			case SpecialType.System_Single:
			case SpecialType.System_Double:
				value.kind_ = "Float";
				break;
			case SpecialType.System_Boolean:
				value.kind_ = "Bool";
				break;
			case SpecialType.System_String:
				value.kind_ = "String";
				break;
			case SpecialType.System_Int64:
			case SpecialType.System_UInt64:
				value.unsupportedReason_ = "64 ビット整数（まだ未対応）の";
				break;
			default:
				if (type.TypeKind == TypeKind.Enum)
				{
					value.kind_ = "Enum";
				}
				else
				{
					switch (type.ToDisplayString())
					{
					case "SeedCore.Vector2":
						value.kind_ = "Vector2";
						break;
					case "SeedCore.Vector3":
						value.kind_ = "Vector3";
						break;
					case "SeedCore.Color":
						value.kind_ = "Color";
						break;
					case "SeedCore.Vector4":
						value.unsupportedReason_ = "Vector4（まだ未対応）の";
						break;
					}
				}
				break;
			}

			if (value.kind_ == null)
			{
				return value;
			}

			switch (value.kind_)
			{
			case "Bool":
				value.size_ = 1;
				value.alignment_ = 1;
				break;
			case "Vector2":
				value.size_ = 8;
				value.alignment_ = 4;
				break;
			case "Vector3":
				value.size_ = 12;
				value.alignment_ = 4;
				break;
			case "Color":
				value.size_ = 16;
				value.alignment_ = 4;
				break;
			case "String":
				value.size_ = 16;
				value.alignment_ = 8;
				break;
			default:
				value.size_ = 4;
				value.alignment_ = 4;
				break;
			}
			return value;
		}

		private sealed class ValueKind
		{
			internal String kind_;

			internal Int32 size_;

			internal Int32 alignment_;

			internal String unsupportedReason_;

			internal String typeName_;

			internal String Read(String address)
			{
				switch (kind_)
				{
				case "Int":
				case "Enum":
					return $"({typeName_})(*(global::System.Int32*){address})";
				case "Float":
					return $"({typeName_})(*(global::System.Single*){address})";
				case "Bool":
					return $"*(global::System.Byte*){address} != 0";
				default:
					return $"*({typeName_}*){address}";
				}
			}

			internal String Write(String address, String source)
			{
				switch (kind_)
				{
				case "Int":
				case "Enum":
					return $"*(global::System.Int32*){address} = (global::System.Int32){source};";
				case "Float":
					return $"*(global::System.Single*){address} = (global::System.Single){source};";
				case "Bool":
					return $"*(global::System.Byte*){address} = {source} ? (global::System.Byte)1 : (global::System.Byte)0;";
				default:
					return $"*({typeName_}*){address} = {source};";
				}
			}
		}

		private sealed class ConditionRewriter : CSharpSyntaxRewriter
		{
			internal String nonPublic_;

			private readonly Dictionary<String, ISymbol> members_;

			internal ConditionRewriter(Dictionary<String, ISymbol> members)
			{
				members_ = members;
			}

			public override SyntaxNode VisitIdentifierName(IdentifierNameSyntax node)
			{
				if (node.Parent is MemberAccessExpressionSyntax memberAccess && memberAccess.Name == node)
				{
					return node;
				}

				if (!members_.TryGetValue(node.Identifier.Text, out ISymbol member))
				{
					return node;
				}

				if (member.DeclaredAccessibility != Accessibility.Public)
				{
					nonPublic_ = nonPublic_ ?? member.Name;
				}

				return SyntaxFactory.MemberAccessExpression(SyntaxKind.SimpleMemberAccessExpression, SyntaxFactory.IdentifierName("o"), node.WithoutTrivia()).WithTriviaFrom(node);
			}
		}
	}
}
