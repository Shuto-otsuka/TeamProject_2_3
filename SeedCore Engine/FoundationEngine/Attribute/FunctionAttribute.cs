using System;

namespace SeedCore
{
    [AttributeUsage(AttributeTargets.Method, Inherited = true, AllowMultiple = false)]
    public sealed class SeedFunctionAttribute : Attribute
    {
        public SeedFunctionAttribute()
        {
            /// No Code
        }
    }
}
