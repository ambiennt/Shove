#include "main.h"
#include <dllentry.h>

class ShoveCommand : public Command {

    Vec3 getRotationRelativeDelta(double yaw) const {
        yaw = (double)((yaw + 90.0) * INV_RADIAN_DEGREES);
        double rx = (double)std::cos(yaw);
        double rz = (double)std::sin(yaw);
        return Vec3((float)(rx * this->x), this->y, (float)(rz * this->x));
    }

public:
    ShoveCommand() {
        this->selector = {};
        this->x = 0.f;
        this->y = 0.f;
        this->z = 0.f;
        this->deltaType = DeltaType::WORLD_RELATIVE;
    }

    enum class DeltaType : int32_t {
        WORLD_RELATIVE = 0,
        ROTATION_RELATIVE = 1,
    };

    CommandSelector<Actor> selector;
    float x, y, z;
    DeltaType deltaType;

    void execute(CommandOrigin const &origin, CommandOutput &output) {

        auto selectedEntities = this->selector.results(origin);

        if (selectedEntities.empty()) {
            return output.error("No targets matched selector");
        }

        SetActorMotionPacket pkt;
        Vec3 delta{};
        int32_t newResultCount = 0;

        if (this->deltaType == DeltaType::ROTATION_RELATIVE) {
            for (auto entity : selectedEntities) {
                if (entity->isInstanceOfMob()) {
                    
                    delta = this->getRotationRelativeDelta((double)entity->mRot.y);

                    if (entity->isInstanceOfPlayer()) {
                        pkt.mRuntimeId = entity->mRuntimeID;
                        pkt.mMotion = delta;
                        ((Player*)entity)->sendNetworkPacket(pkt);
                    }
                    else {
                        entity->mStateVectorComponent.mPosDelta = delta;
                    }
                    newResultCount++;
                }
            }
        }
        else {
            delta = { this->x, this->y, this->z };
            pkt.mMotion = delta;

            for (auto entity : selectedEntities) {
                if (entity->isInstanceOfMob()) {
                    if (entity->isInstanceOfPlayer()) {
                        pkt.mRuntimeId = entity->mRuntimeID;
                        ((Player*)entity)->sendNetworkPacket(pkt);
                    }
                    else {
                        entity->mStateVectorComponent.mPosDelta = delta;
                    }
                    newResultCount++;
                }
            }
        }
        
        std::stringstream ss;
        ss << "Successfully knocked back " << newResultCount << ((newResultCount == 1) ? " entity" : " entities") <<
            " with a " << ((this->deltaType == DeltaType::ROTATION_RELATIVE) ? "rotation" : "world") << "-relative delta of { " <<
            std::setw(0) << delta.x << ", " << std::setw(0) << delta.y << ", " << std::setw(0) << delta.z << " }";
        output.success(ss.str());
    }

    static void setup(CommandRegistry *registry) {
        using namespace commands;
        
        addEnum<DeltaType>(registry, "worldRelativeType", {
            { "world_relative", DeltaType::WORLD_RELATIVE }
        });

        addEnum<DeltaType>(registry, "rotationRelativeType", {
            { "rotation_relative", DeltaType::ROTATION_RELATIVE }
        });

        registry->registerCommand(
            "shove", "Knocks an entity back in a specified direction.",
            CommandPermissionLevel::GameMasters, CommandFlagUsage, CommandFlagNone);

        registry->registerOverload<ShoveCommand>("shove",
            mandatory(&ShoveCommand::selector, "target"),
            mandatory<CommandParameterDataType::ENUM>(&ShoveCommand::deltaType, "world_relative", "worldRelativeType"),
            mandatory(&ShoveCommand::x, "x"),
            mandatory(&ShoveCommand::y, "y"),
            mandatory(&ShoveCommand::z, "z")
        );

        registry->registerOverload<ShoveCommand>("shove",
            mandatory(&ShoveCommand::selector, "target"),
            mandatory<CommandParameterDataType::ENUM>(&ShoveCommand::deltaType, "rotation_relative", "rotationRelativeType"),
            mandatory(&ShoveCommand::x, "x"),
            mandatory(&ShoveCommand::y, "y")
        );
    }
};

void dllenter() {}
void dllexit() {}
void PreInit() {
    Mod::CommandSupport::GetInstance().AddListener(SIG("loaded"), &ShoveCommand::setup);
}
void PostInit() {}