using System;

namespace SeedCore
{
    [AttributeUsage(AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
    public sealed class SeedReflectionFieldAttribute : Attribute
    {
        internal String displayName_;

        public SeedReflectionFieldAttribute()
        {
            /// No Code
        }

        public SeedReflectionFieldAttribute(String displayName)
        {
            displayName_ = displayName;
        }
    }

    [AttributeUsage(AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
    public sealed class SeedReflectionRangeAttribute : Attribute
    {
        internal String displayName_;

        internal Double min_;

        internal Double max_;

        public SeedReflectionRangeAttribute(Double min, Double max)
        {
            min_ = min;
            max_ = max;
        }

        public SeedReflectionRangeAttribute(String displayName, Double min, Double max)
        {
            displayName_ = displayName;
            min_ = min;
            max_ = max;
        }
    }

    [AttributeUsage(AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
    public sealed class SeedReflectionFieldConditionAttribute : Attribute
    {
        internal String condition_;

        public SeedReflectionFieldConditionAttribute(String condition)
        {
            condition_ = condition;
        }
    }

    [AttributeUsage(AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
    public sealed class SeedSerializeFieldAttribute : Attribute
    {
        public SeedSerializeFieldAttribute()
        {
            /// No Code
        }
    }
}
