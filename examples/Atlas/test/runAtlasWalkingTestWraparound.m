function runAtlasWalkingTestWraparound()

path_handle = addpathTemporary(fullfile(getDrakePath,'examples','Atlas'));
runAtlasWalkingSplit([], struct('initial_pose', [0;0;0;0;0;-pi+pi/16],...
                                'navgoal', [0;0;0;0;0;pi-pi/16]));
end

