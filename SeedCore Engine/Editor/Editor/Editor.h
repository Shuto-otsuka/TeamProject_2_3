#pragma once

namespace SeedCore
{
    struct EditorContext;

    class Editor : public NonTransferable
    {
    public:
        Editor(EditorContext& context);
        ~Editor();

        void Tick(Float deltaTime);

        void Draw();

    private:
        class Impl;

        ResourcePtr<Impl> impl_;
    };
}
