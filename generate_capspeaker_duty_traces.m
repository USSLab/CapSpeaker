%% Read the original audio, and the audio length is complemented to 5 seconds.
clc;clear;
tic;
filelist = dir('voice_command'); % input dir
for ff = 3:length(filelist)
    %% Audio file reading and amplifying
    filename = filelist(ff).name;
    time_len = 2;
    [data_read, fs] = audioread(['voice_command\',filename]);
    data_read = data_read / max(abs(data_read));
    if(length(data_read) > time_len * fs)
         data = data_read(1:time_len * fs);
    else
        data = zeros(time_len * fs, 1);
        data(1:length(data_read)) = data_read;
    end
    
    %% Remove the part of original audio greater than 2000 Hz
    data = mean(data, 2);
    N = length(data);
    t_canon = 0 : 1/fs : (N-1)/fs;
    fx_canon = 0 : fs/N : fs - fs/N;
    data_fft = abs(fft(data));
    % filter low frequency bands
    pass_low = 20;
    pass_high = 2000;   
    ts = timeseries(data, t_canon);
    ts_filtered = idealfilter(ts, [pass_low, pass_high], 'pass');
    data_filtered = ts_filtered.Data;

    %% Set PWM parameters
    target_frequency = 24000; % PWM carrier frequency
    duty_upper_bound = 0.99; % maximum duty cycle
    duty_lower_bound = 0.01; % minimum duty cycle
    full_busy = 2047; % Timer accuracy decreased by 1
    full_idle = 2047;% Timer accuracy decreased by 1
    sample_rate = target_frequency * 5; % used to calculate the sampling rate for computing PWM duty cycle

    period = 1 / target_frequency; % PWM period
    N = time_len * sample_rate;
    t = 0 : 1 / sample_rate : (N - 1) / sample_rate;

    %% The original audio is interpolated for easy calculation
    data_interp = interp1(t_canon', data_filtered, t', 'nearest');
    target_wave = data_interp;
    target_wave(find(isnan(target_wave))) = 0;

    %% Calculate PWM duty cycle
    period_samples = round(period * sample_rate);
    period_num = floor(N / period_samples);
    duty = zeros(period_num, 1);
    pwm_wave = zeros(N, 1);
    for ii = 1:period_num
        strength = target_wave((ii-1) * period_samples + 1 );
        duty(ii) = (strength + 1) / 2 * (duty_upper_bound - duty_lower_bound) + duty_lower_bound;
        busy_samples = round(duty(ii) * period_samples);
        pwm_wave((ii-1) * period_samples + 1: (ii-1) * period_samples + busy_samples) = 1;
    end
    busy_time = duty * full_busy; % us
    idle_time = (1 - duty) * full_idle;
    results = [round(busy_time)];
    %% Write duty cycle traces to txt files
    fid = fopen(['traces\',filename(1:end-4),'_duty_cycle_24k.txt'], 'w');
    fprintf(fid, "a={");
    for i=1:2:length(results)
        fprintf(fid, "%d,", results(i));   
        if(mod(i, 20)== 1) 
            fprintf(fid, "\n");
        end
    end
    fprintf(fid, "%d", results(end));
    fprintf(fid, "};");
    fclose(fid);
    
end
toc