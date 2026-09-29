using System;

namespace SeedCore
{
    [AttributeUsage(AttributeTargets.Class, Inherited = true, AllowMultiple = false)]
    public sealed class SeedCategoryAttribute : Attribute
    {
        internal String category_;

        public SeedCategoryAttribute(String category)
        {
            category_ = category;
        }
    }
}
