#############################
#NVIDIA  All Rights Reserved
#Haoyu Yang 
#Design Automation Research
#Last Update: Aug 14 2025
#############################
import cv2
import numpy as np
import torch
from torch import nn
from lithography import DEVICE, Lithography

class Solver():
    def __init__(self, image_path, avepool_kernel=5,morph=0,scale_factor=1,pixel_size=1,all_tech=0):
        
        self.image_path = image_path
        self.litho = Lithography(target_path=image_path, avepool_kernel=avepool_kernel, morph=morph,scale_factor=scale_factor,pixel_size=pixel_size).to(DEVICE)
        self.optimizer = torch.optim.SGD(self.litho.parameters(), lr=1)
        #self.optimizer = Prodigy(self.litho.parameters(), lr=1, weight_decay=0)
        self.iteration = 0
        self.target = torch.tensor(cv2.imread(image_path, -1))/255.0 
        self.target_s = torch.tensor(cv2.imread(image_path, -1))/255.0
        self.mask_dim1, self.mask_dim2 = self.target.shape
        self.target = self.target.view(1,1,self.mask_dim1,self.mask_dim2).to(DEVICE)
        self.mask_dim1_s = self.mask_dim1//self.litho.scale_factor
        self.mask_dim2_s = self.mask_dim2//self.litho.scale_factor
        self.target_s = nn.functional.avg_pool2d(self.target_s.view(1,1,self.mask_dim1,self.mask_dim2).to(DEVICE), self.litho.scale_factor)
        self.lowpass = 2
        self.lowpass_mask = torch.ones(1,1,self.mask_dim1_s, self.mask_dim2_s).to(DEVICE)
        self.lowpass_offset = self.mask_dim1_s//2-self.lowpass//2
        self.lowpass_mask[:,:,self.lowpass_offset:self.lowpass_offset+self.lowpass, self.lowpass_offset:self.lowpass_offset+self.lowpass]=0
        self.loss =0
        self.loss_l2=0
        self.loss_pvb=0
        self.loss_pvb_i =0
        self.loss_pvb_o =0
        self.loss_pvb = 0
        self.loss_lowpass_reg=0
        self.lowpass_reg_lambda = 1e-3

    def backward(self):

        self.loss.backward()



    def forward(self):

        self.mask = self.litho.mask_s.data
        
        self.nominal, self.outer, self.inner = self.litho.forward()
        self.i_mask_fft = self.litho.i_mask_fft
        self.iteration = self.iteration + 1
        self.litho.iter = self.iteration

    def optimize(self):
        with torch.autocast(device_type=DEVICE.type, enabled=DEVICE.type == "cuda"):
            self.forward()
            self.loss_l2 = l2_loss(self.outer, self.target_s)
            self.loss_pvb = l2_loss(self.inner, self.outer) 
            self.loss_lowpass_reg = torch.norm(torch.abs(self.i_mask_fft*self.lowpass_mask))
            self.loss = self.loss_l2 + self.loss_pvb + self.lowpass_reg_lambda*self.loss_lowpass_reg
        self.optimizer.zero_grad()
        self.backward()
        self.optimizer.step()


        

def l2_loss(x, y):
    return torch.sum(torch.pow((x - y), 2))

def morph_close_cv2(im, kernel=10):
    k=cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (kernel,kernel))
    b=cv2.dilate(im,k)
    b=cv2.erode(b,k)
    return b 

def morph_open_cv2(im, kernel=20):
    k=cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (kernel,kernel))
    b=cv2.erode(im,k)
    b=cv2.dilate(b,k)
    return b 

#corner retargeting


def corner_retargeting_morph(image, kernel_convex=20, kernel_concave=None,output_filename=None):
    if kernel_concave==None:
        kernel_concave=kernel_convex
    image_open = morph_open_cv2(image,kernel_convex)
    image_close= morph_close_cv2(image,kernel_concave)
    image= image_open + image_close - image
    if output_filename:
        cv2.imwrite(output_filename, image * 255)  # Convert to 0-255 scale for PNG
    return image
