function [X, foot_goals] = createInitialSteps(biped, x0, poses, options)

  debug = false;
  biped.getNextStepID(true);

  q0 = x0(1:end/2);
  foot_orig = biped.feetPosition(q0);
  sizecheck(poses, [6,1]);

  % poses(6, poses(6,:) < -pi) = poses(6, poses(6,:) < -pi) + 2 * pi;
  % poses(6, poses(6,:) > pi) = poses(6, poses(6,:) > pi) - 2 * pi;
    
  if options.right_foot_lead
    X(1) = struct('pos', biped.footOrig2Contact(foot_orig.right, 'center', 1), 'step_speed', 0, 'step_height', 0, 'id', biped.getNextStepID(), 'pos_fixed', ones(6,1), 'is_right_foot', true, 'is_in_contact', true);
    X(2) = struct('pos', biped.footOrig2Contact(foot_orig.left, 'center', 0), 'step_speed', 0, 'step_height', 0, 'id', biped.getNextStepID(), 'pos_fixed', ones(6,1), 'is_right_foot', false, 'is_in_contact', true);
  else
    X(1) = struct('pos', biped.footOrig2Contact(foot_orig.left, 'center', 1), 'step_speed', 0, 'step_height', 0, 'id', biped.getNextStepID(), 'pos_fixed', ones(6,1), 'is_right_foot', false, 'is_in_contact', true);
    X(2) = struct('pos', biped.footOrig2Contact(foot_orig.right, 'center', 0), 'step_speed', 0, 'step_height', 0, 'id', biped.getNextStepID(), 'pos_fixed', ones(6,1), 'is_right_foot', true, 'is_in_contact', true);
  end
  
  p0 = [mean([X(1).pos(1:3), X(2).pos(1:3)], 2); X(1).pos(4:6)];
  % poses(6) = p0(6) + angleDiff(p0(6), poses(6));
  unwrapped = unwrap([p0(6), poses(6)]);
  poses(6) = unwrapped(2);
  poses(3,:) = p0(3);
  if ~options.ignore_terrain
    poses = biped.checkTerrain(poses);
  end
  foot_goals = struct('right', biped.stepCenter2FootCenter(poses(1:6,end), 1), 'left', biped.stepCenter2FootCenter(poses(1:6,end), 0));

  if options.follow_spline
    traj = BezierTraj([p0, poses]);
  else
    traj = DirectTraj([p0, poses]);
  end
  ls = linspace(0, 1);
  xy = traj.eval(ls);

  [lambdas, infeasibility, foot_centers] = scanWalkingTerrain(biped, traj, p0);
  if options.ignore_terrain
    infeasibility.right = zeros(size(infeasibility.right));
    infeasibility.left = zeros(size(infeasibility.left));
    foot_centers.right(3,:) = p0(3);
    foot_centers.left(3,:) = p0(3);
  end

  if debug
    plot_lcm_points([xy(1,:)', xy(2,:)', xy(3,:)'], repmat([0, 0, 1], length(ls), 1), 50, 'Foostep Spline', 2, 1);
  end
  
  last_ndx = struct('right', 1, 'left', 1);
  
stall = struct('right', 0, 'left', 0);
aborted = false;
  
  while (1)
    is_right_foot = ~X(end).is_right_foot;
    if is_right_foot
      m_foot = 'right';
      s_foot = 'left';
    else
      m_foot = 'left';
      s_foot = 'right';
    end
    lambda_n = 1;
    nom_forward_step = biped.nom_forward_step;

    potential_poses = foot_centers.(m_foot)(:, last_ndx.(m_foot):end);
    reach = biped.checkStepFeasibility(repmat(X(end).pos, 1, size(potential_poses, 2)),...
      potential_poses, repmat(~is_right_foot, 1, size(potential_poses, 2)), nom_forward_step);
    reach = reshape(reach, [], size(potential_poses, 2));
    valid_pose_ndx = find(max(reach, [], 1) <= 0 & ~infeasibility.(m_foot)(last_ndx.(m_foot):end)) + last_ndx.(m_foot) - 1;
    if length(valid_pose_ndx) < 1
      aborted = true;
      break
    end
    next_ndx = valid_pose_ndx(end);

    if all(abs(foot_centers.(m_foot)(:, next_ndx) - foot_centers.(m_foot)(:,last_ndx.(m_foot))) < [0.01;0.01;1;0.1;0.1;0.1]) && stall.(s_foot)
      reach = biped.checkStepFeasibility(repmat(X(end).pos, 1, size(potential_poses, 2)),...
      potential_poses, repmat(~is_right_foot, 1, size(potential_poses, 2)), biped.max_forward_step);
      reach = reshape(reach, [], size(potential_poses, 2));
      valid_pose_ndx = find(max(reach, [], 1) <= 0 & ~infeasibility.(m_foot)(last_ndx.(m_foot):end)) + last_ndx.(m_foot) - 1;
      next_ndx = valid_pose_ndx(end);
    end

    if all(abs(foot_centers.(m_foot)(:, next_ndx) - foot_centers.(m_foot)(:,last_ndx.(m_foot))) < [0.01;0.01;1;0.1;0.1;0.1])
      stall.(m_foot) = stall.(m_foot) + 1;
      if stall.(m_foot) >= 2 || (stall.(m_foot) > 0 && stall.(s_foot) > 0)
        aborted = true;
        break
      end
    end
    if options.ignore_terrain
      pos_n = foot_centers.(m_foot)(:, next_ndx);
    else
      pos_n = biped.checkTerrain(foot_centers.(m_foot)(:, next_ndx));
    end
    last_ndx.(m_foot) = next_ndx;

    if is_right_foot
      last_lambda.right = lambda_n;
    else
      last_lambda.left = lambda_n;
    end
    % if length(X) == 2
    %   last_pos = X(end-1).pos;
    % else
    %   last_pos = X(end-2).pos;
    % end
    % for j = 4:6
    %   pos_n(j) = last_pos(j) + angleDiff(last_pos(j), pos_n(j));
    % end
    % apex_pos = biped.get_apex_pos(last_pos, pos_n);
    % X(end+1) = struct('pos', apex_pos, 'step_speed', 0, 'id', biped.getNextStepID(), 'pos_fixed', zeros(6, 1), 'is_right_foot', is_right_foot, 'is_in_contact', false);
    X(end+1) = struct('pos', pos_n, 'step_speed', 0, 'step_height', 0, 'id', biped.getNextStepID(), 'pos_fixed', zeros(6, 1), 'is_right_foot', is_right_foot, 'is_in_contact', true);

    if X(end).is_right_foot
      goal = foot_goals.right;
    else
      goal = foot_goals.left;
    end
    if (all(abs(X(end).pos - goal) < 0.05) || (length(X) - 2) >= options.max_num_steps - 1) && ((length(X) - 2) >= options.min_num_steps - 1)
      break
    end
  end
  
  if aborted && length(X) > 3
    % If we had to give up, then lose the last (unproductive) step
    X = X(1:end-1);
  end

  % Add final step (and multiple steps in place, if necessary to meet min number of steps)
  if length(X) > 2 || (length(X) - 2) < options.min_num_steps
    while (1)
      final_center = biped.footCenter2StepCenter(X(end).pos, X(end).is_right_foot);
      final_pos = biped.stepCenter2FootCenter(final_center, ~X(end).is_right_foot);
      is_right_foot = ~X(end).is_right_foot;

      X(end+1) = struct('pos', final_pos, 'step_speed', 0, 'step_height', 0, 'id', biped.getNextStepID(), 'pos_fixed', zeros(6, 1), 'is_right_foot', is_right_foot, 'is_in_contact', true);
      if (length(X) - 2) >= options.min_num_steps
        break
      end
    end
  end
 
  for j = 1:length(X)
    X(j).step_speed = options.step_speed;
    X(j).step_height = options.step_height;
  end
  
end
