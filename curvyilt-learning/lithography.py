#############################
#NVIDIA  All Rights Reserved
#Haoyu Yang 
#Design Automation Research
#Last Update: Aug 14 2025
#############################
from pathlib import Path
import cv2
import numpy as np
import torch
from torch import nn
from kornia.morphology import opening, closing

DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")
KERNELS = Path(__file__).resolve().parent / "tcc"

def get_kernels():
    # Load and process kernel_head
    kernel_head = np.load(KERNELS / "optKernel_bc.npy")
    kernel_head = kernel_head[:, :24]

    # Load and process kernel_scale
    kernel_scale = np.load(KERNELS / "optKernel_scale.npy")
    kernel_scale = kernel_scale[:, :24]
    a, b = kernel_scale.shape
    kernel_scale = kernel_scale.reshape(a, b, 1, 1)

    # Extract focus and defocus components
    kernels_fft_focus = kernel_head[0]
    kernels_fft_defocus = kernel_head[1]
    kernels_scale_focus = kernel_scale[0]
    kernels_scale_defocus = kernel_scale[1]

    return kernels_fft_focus, kernels_fft_defocus, kernels_scale_focus, kernels_scale_defocus
class Lithography(nn.Module):
    def __init__(self, target_path, mask_steepness=4, resist_th =0.225, resist_steepness=50, mask_shift=0.5, pvb_coefficient=0, max_dose=1.02, min_dose=0.98, avepool_kernel=3, morph=0, scale_factor=8, pixel_size=1, max_iter=None):
        super().__init__()
        self.target_image = torch.tensor(cv2.imread(target_path, -1))/255.0 

        self.mask_dim1, self.mask_dim2 = self.target_image.shape
        self.target_image = self.target_image.view(1,1,self.mask_dim1,self.mask_dim2).to(DEVICE)
        self.mask = nn.Parameter(self.target_image)    
        self.fo, self.defo, self.fo_scale, self.defo_scale = get_kernels()

        self.kernel_focus = torch.tensor(self.fo).to(DEVICE)
        self.kernel_focus_scale = torch.tensor(self.fo_scale).to(DEVICE)
        self.kernel_defocus = torch.tensor(self.defo).to(DEVICE)
        self.kernel_defocus_scale = torch.tensor(self.defo_scale).to(DEVICE)


        self.kernel_num, self.kernel_dim1, self.kernel_dim2 = self.fo.shape # 24 35 35
        self.offset = self.mask_dim1//2-self.kernel_dim1//2
        self.max_dose = max_dose
        self.min_dose = min_dose
        self.resist_steepness = resist_steepness
        self.mask_steepness = mask_steepness
        self.resist_th = resist_th
        self.mask_shift = mask_shift
        self.scale_factor = scale_factor
        self.mask_dim1_s = self.mask_dim1//self.scale_factor
        self.mask_dim2_s = self.mask_dim2//self.scale_factor
        self.target_image_s = nn.functional.avg_pool2d(torch.tensor(cv2.imread(target_path, -1)).view(1,1,self.mask_dim1,self.mask_dim2).to(DEVICE)/255.0, self.scale_factor)
        self.mask_s      = nn.Parameter(self.target_image_s)
        self.avepool  = nn.AvgPool2d(kernel_size=avepool_kernel, stride=1, padding = avepool_kernel//2)
        self.offset_s = self.mask_dim1_s//2-self.kernel_dim1//2

        self.max_iter=max_iter
        #morphlogic for mask simplification
        self.morph = morph
        if self.morph>0:
            self.morph_kernel_opt_opening =torch.tensor(cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (morph,morph)).astype(np.float32)).to(DEVICE)
            self.morph_kernel_opt_closing =torch.tensor(cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (morph+2,morph+2)).astype(np.float32)).to(DEVICE)
            self.morph_kernel_opening = torch.tensor(cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (morph*scale_factor+1,morph*scale_factor+1)).astype(np.float32)).to(DEVICE)
            self.morph_kernel_closing = torch.tensor(cv2.getStructuringElement(cv2.MORPH_ELLIPSE, ((morph-2)*self.scale_factor+1,(morph-2)*self.scale_factor+1)).astype(np.float32)).to(DEVICE)
        self.iter=0

    def standalone_mask_morph(self):
        mask = self.avepool(self.mask_s)
        mask = torch.sigmoid(self.mask_steepness*(mask-self.mask_shift))
        mask_o = opening(mask, self.morph_kernel_opt_opening)
        mask_c = closing(mask, self.morph_kernel_opt_closing)
        mask = mask_o+mask_c-mask
        return mask

    def forward_test(self,use_morph=False):  
        mask = torch.zeros_like(self.target_image).to(DEVICE)
        cmask = self.mask.data
        #print(mask.shape, cmask.shape)
        mask[self.mask.data>=0.5]=1.0
        mask[self.mask.data<0.5]=0.0
        if self.morph>0 and use_morph:
            mask_o = opening(mask, self.morph_kernel_opening, engine="convolution")
            mask_c = closing(mask, self.morph_kernel_closing, engine="convolution")
            mask = mask_o+mask_c-mask
            mask = opening(mask, self.morph_kernel_opening, engine="convolution")
            mask = closing(mask, self.morph_kernel_closing, engine="convolution")

        mask_fft = torch.fft.fftshift(torch.fft.fft2(mask)) 
        mask_fft = torch.repeat_interleave(mask_fft, self.kernel_num, 1) 


        mask_fft_max = torch.fft.fftshift(torch.fft.fft2(mask*self.max_dose)) 
        mask_fft_max = torch.repeat_interleave(mask_fft_max, self.kernel_num, 1) 

        mask_fft_min = torch.fft.fftshift(torch.fft.fft2(mask*self.min_dose)) 
        mask_fft_min = torch.repeat_interleave(mask_fft_min, self.kernel_num, 1) 

        x_out = torch.view_as_complex(torch.zeros((1, self.kernel_num, self.mask_dim1, self.mask_dim2, 2), dtype=torch.float32).to(DEVICE))

        x_out_max = torch.view_as_complex(torch.zeros((1, self.kernel_num, self.mask_dim1, self.mask_dim2, 2), dtype=torch.float32).to(DEVICE))

        x_out_min = torch.view_as_complex(torch.zeros((1, self.kernel_num, self.mask_dim1, self.mask_dim2, 2), dtype=torch.float32).to(DEVICE))


        x_out[:, :, self.offset: self.offset+self.kernel_dim1, self.offset: self.offset+self.kernel_dim2] = mask_fft[:, :, self.offset: self.offset+self.kernel_dim1, self.offset: self.offset+self.kernel_dim2] * self.kernel_focus
        x_out = torch.fft.ifft2(x_out)
        x_out = x_out.real*x_out.real + x_out.imag*x_out.imag
        x_out = x_out * self.kernel_focus_scale
        x_out = torch.sum(x_out, axis=1, keepdims=True)
        self.aerial = x_out
        x_out = torch.sigmoid(self.resist_steepness*(x_out-self.resist_th))


        x_out_max[:, :, self.offset: self.offset+self.kernel_dim1, self.offset: self.offset+self.kernel_dim2] = mask_fft_max[:, :, self.offset: self.offset+self.kernel_dim1, self.offset: self.offset+self.kernel_dim2] * self.kernel_focus
        x_out_max = torch.fft.ifft2(x_out_max)
        x_out_max = x_out_max.real*x_out_max.real + x_out_max.imag*x_out_max.imag
        x_out_max = x_out_max * self.kernel_focus_scale
        x_out_max = torch.sum(x_out_max, axis=1, keepdims=True)
        x_out_max = torch.sigmoid(self.resist_steepness*(x_out_max-self.resist_th))

        x_out_min[:, :, self.offset: self.offset+self.kernel_dim1, self.offset: self.offset+self.kernel_dim2] = mask_fft_min[:, :, self.offset: self.offset+self.kernel_dim1, self.offset: self.offset+self.kernel_dim2] * self.kernel_defocus
        x_out_min = torch.fft.ifft2(x_out_min)
        x_out_min = x_out_min.real*x_out_min.real + x_out_min.imag*x_out_min.imag
        x_out_min = x_out_min * self.kernel_defocus_scale
        x_out_min = torch.sum(x_out_min, axis=1, keepdims=True)
        x_out_min = torch.sigmoid(self.resist_steepness*(x_out_min-self.resist_th))

        x_out[x_out>=0.5]=1.0
        x_out[x_out<0.5]=0.0
        x_out_max[x_out_max>=0.5]=1.0
        x_out_max[x_out_max<0.5]=0.0
        x_out_min[x_out_min>=0.5]=1.0
        x_out_min[x_out_min<0.5]=0.0


        return mask, cmask, x_out, x_out_max, x_out_min
    
    
    def forward(self,): 

        mask = self.avepool(self.mask_s) 

        mask = torch.sigmoid(self.mask_steepness*(mask-self.mask_shift))

        if self.morph>0 and self.iter % 2==0 and self.iter >50:
            mask_o = opening(mask, self.morph_kernel_opt_opening, engine="convolution")
            mask_c = closing(mask, self.morph_kernel_opt_closing, engine="convolution")
            mask = mask_o+mask_c-mask


        mask_fft = torch.fft.fftshift(torch.fft.fft2(mask)) 
        self.i_mask_fft = mask_fft
        mask_fft = torch.repeat_interleave(mask_fft, self.kernel_num, 1) 
        mask_fft_max = mask_fft*self.max_dose
        mask_fft_min = mask_fft*self.min_dose


        x_out = torch.view_as_complex(torch.zeros((1, self.kernel_num, self.mask_dim1_s, self.mask_dim2_s, 2), dtype=torch.float32).to(DEVICE))
        x_out_max = torch.view_as_complex(torch.zeros((1, self.kernel_num, self.mask_dim1_s, self.mask_dim2_s, 2), dtype=torch.float32).to(DEVICE))
        x_out_min = torch.view_as_complex(torch.zeros((1, self.kernel_num, self.mask_dim1_s, self.mask_dim2_s, 2), dtype=torch.float32).to(DEVICE))


        x_out[:, :, self.offset_s: self.offset_s+self.kernel_dim1, self.offset_s: self.offset_s+self.kernel_dim2] = mask_fft[:, :, self.offset_s: self.offset_s+self.kernel_dim1, self.offset_s: self.offset_s+self.kernel_dim2] * self.kernel_focus
        x_out = torch.fft.ifft2(x_out)
        x_out = x_out.real*x_out.real + x_out.imag*x_out.imag
        x_out = x_out * self.kernel_focus_scale
        x_out = torch.sum(x_out, axis=1, keepdims=True)
        x_out = torch.sigmoid(self.resist_steepness*(x_out-self.resist_th))


        x_out_max[:, :, self.offset_s: self.offset_s+self.kernel_dim1, self.offset_s: self.offset_s+self.kernel_dim2] = mask_fft_max[:, :, self.offset_s: self.offset_s+self.kernel_dim1, self.offset_s: self.offset_s+self.kernel_dim2] * self.kernel_focus
        x_out_max = torch.fft.ifft2(x_out_max)
        x_out_max = x_out_max.real*x_out_max.real + x_out_max.imag*x_out_max.imag
        x_out_max = x_out_max * self.kernel_focus_scale
        x_out_max = torch.sum(x_out_max, axis=1, keepdims=True)
        x_out_max = torch.sigmoid(self.resist_steepness*(x_out_max-self.resist_th))

        x_out_min[:, :, self.offset_s: self.offset_s+self.kernel_dim1, self.offset_s: self.offset_s+self.kernel_dim2] = mask_fft_min[:, :, self.offset_s: self.offset_s+self.kernel_dim1, self.offset_s: self.offset_s+self.kernel_dim2] * self.kernel_defocus
        x_out_min = torch.fft.ifft2(x_out_min)
        x_out_min = x_out_min.real*x_out_min.real + x_out_min.imag*x_out_min.imag
        x_out_min = x_out_min * self.kernel_defocus_scale
        x_out_min = torch.sum(x_out_min, axis=1, keepdims=True)
        x_out_min = torch.sigmoid(self.resist_steepness*(x_out_min-self.resist_th))


        return x_out, x_out_max, x_out_min
