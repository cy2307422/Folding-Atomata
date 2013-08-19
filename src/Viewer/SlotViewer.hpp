
#ifndef SLOT_VIEWER_HPP
#define SLOT_VIEWER_HPP

#include "../Trajectory/Trajectory.hpp"
#include "../Modeling/Mesh/Mesh.hpp"
#include "../Modeling/DataBuffers/ColorBuffer.hpp"
#include "../World/Scene.hpp"

/*
SlotViewer handles the viewing of the protein from a particular slot.
The protein is displayed as soon as the topology comes in, and then explodes
into position once the first checkpoint becomes available. From there, it iterates
through all checkpoints, and then jumps to the first checkpoint and repeats.
If Tracers are enabled, it shows them.
*/

typedef std::pair<ProgramPtr, ColorPtr> AtomModelInfo;

class SlotViewer
{
    public:
        SlotViewer(const TrajectoryPtr& trajectory, 
                   const std::shared_ptr<Scene>& scene
        );
        void update(int deltaTime);

    public:
        const float ATOM_SCALE = 0.1f;
        const float BOND_SCALE = 0.01f;

    private:
        std::shared_ptr<Mesh> generateAtomMesh();
        std::shared_ptr<Mesh> generateBondMesh();

        void addAllAtoms();
        void addAllBonds();

        ModelPtr addAtom(const AtomPtr& atom, const glm::mat4& matrix);
        void addBond(const BondPtr& bond, const ModelPtr& model);
        
        ModelPtr generateAtomModel(const ColorPtr& cBuffer,
                                   const glm::mat4& matrix
        );

        glm::mat4 alignBetween(const glm::vec3& a, const glm::vec3& b);
        float getDotProduct(const glm::vec3& vecA, const glm::vec3& vecB);
        float getMagnitude(const glm::vec3& vector);

    private:
        TrajectoryPtr trajectory_;
        std::shared_ptr<Scene> scene_;
        std::vector<ModelPtr> atomModels_;
        std::vector<ModelPtr> bondModels_;
};

#endif
